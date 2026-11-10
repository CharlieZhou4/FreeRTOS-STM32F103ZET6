#include "stm32f1xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "print.h"

#define PRINT_BAUD      115200U
#define PRINT_BUF_SIZE  128U
#define PRINT_DMA_IRQ_PRIO 6U  /* must be numerically >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY */

static SemaphoreHandle_t xPrintMutex;     /* serialises tasks sharing the buffer and DMA channel */
static SemaphoreHandle_t xPrintDoneSem;   /* given by the DMA transfer-complete interrupt */
static char cPrintBuf[PRINT_BUF_SIZE];    /* copy of the caller's text, so it may live on the stack */

void vPrintInit(void)
{
    uint32_t ppre2 = (RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos;
    uint32_t pclk2 = (ppre2 & 0x4U) ? SystemCoreClock >> ((ppre2 & 0x3U) + 1U) : SystemCoreClock;

    xPrintMutex = xSemaphoreCreateMutex();
    xPrintDoneSem = xSemaphoreCreateBinary();

    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN | RCC_APB2ENR_AFIOEN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

    /* PA9 = USART1_TX: alternate function push-pull, 50 MHz */
    GPIOA->CRH = (GPIOA->CRH & ~(0xFUL << 4)) | (0xBUL << 4);

    USART1->BRR = (pclk2 + PRINT_BAUD / 2U) / PRINT_BAUD;
    USART1->CR3 = USART_CR3_DMAT;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE;

    /* USART1_TX is served by DMA1 channel 4: memory -> peripheral, 8-bit, memory increment */
    DMA1_Channel4->CPAR = (uint32_t)&USART1->DR;
    DMA1_Channel4->CCR = DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE;
    NVIC_SetPriority(DMA1_Channel4_IRQn, PRINT_DMA_IRQ_PRIO);
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
}

void DMA1_Channel4_IRQHandler(void)
{
    BaseType_t xWoken = pdFALSE;

    if (DMA1->ISR & DMA_ISR_TCIF4) {
        DMA1->IFCR = DMA_IFCR_CGIF4;
        DMA1_Channel4->CCR &= ~DMA_CCR_EN;
        xSemaphoreGiveFromISR(xPrintDoneSem, &xWoken);
    }
    portYIELD_FROM_ISR(xWoken);
}

void vPrintStringPolled(const char *pcString)
{
    while (*pcString) {
        while (!(USART1->SR & USART_SR_TXE)) {}
        USART1->DR = (uint8_t)*pcString++;
    }
    while (!(USART1->SR & USART_SR_TC)) {}
}

void vPrintString(const char *pcString)
{
    /* Before the scheduler runs there is no task to block, so fall back to polling. */
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        vPrintStringPolled(pcString);
        return;
    }

    xSemaphoreTake(xPrintMutex, portMAX_DELAY);
    while (*pcString) {
        uint32_t n = 0;
        while (*pcString && n < PRINT_BUF_SIZE) {
            cPrintBuf[n++] = *pcString++;
        }
        DMA1_Channel4->CMAR = (uint32_t)cPrintBuf;
        DMA1_Channel4->CNDTR = n;
        DMA1_Channel4->CCR |= DMA_CCR_EN;
        xSemaphoreTake(xPrintDoneSem, portMAX_DELAY);  /* task sleeps while DMA feeds the UART */
    }
    xSemaphoreGive(xPrintMutex);
}
