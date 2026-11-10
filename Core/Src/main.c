/* FreeRTOS learning skeleton for STM32F103RCT6: only brings up the clock and the scheduler. */
#include <stdio.h>
#include "stm32f1xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "print.h"
#include "tick_test.h"

void vTask2(void *pvParameters)
{
    const char *pcTaskName = "Task2 is running\r\n";
    volatile uint32_t ul;
    for (;;) 
    {
        vPrintString(pcTaskName);
        for (ul = 0; ul < 10000; ul++) 
        {
        }  /* crude delay */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vTask1(void *pvParameters)
{
    const char *pcTaskName = "Task1 is running\r\n";
    volatile uint32_t ul;
    xTaskCreate(vTask2, "Task2", 1000, NULL, 1, NULL);

    for (;;) 
    {
        vPrintString(pcTaskName);
        for (ul = 0; ul < 10000; ul++) 
        {
        }  /* crude delay */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

volatile uint32_t uldleCycleCount = 0;

static void vPrintStringAndNumber(const char *pcString, uint32_t ulNumber)
{
    char cBuf[64];
    snprintf(cBuf, sizeof cBuf, "%s idle count = %lu\r\n", pcString, (unsigned long)ulNumber);
    vPrintString(cBuf);
}

void vTaskFunction(void *pvParameters)
{
    char *pcTaskName;
    TickType_t xLastWakeTime;
    pcTaskName = (char *)pvParameters;
    xLastWakeTime = xTaskGetTickCount();

    for (;;) 
    {
        vPrintStringAndNumber(pcTaskName, uldleCycleCount);
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(250));

    }
}
void vContinousProcessingTask(void *pvParameters)
{
    char *pcTaskName;
    pcTaskName = (char *)pvParameters;

    for (;;) 
    {
        vPrintString(pcTaskName);
    }
}
void vPeriodicTask(void *pvParameters)
{
    char cMsg[48];
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

    for (;;) 
    {
        snprintf(cMsg, sizeof(cMsg), "[%lu ms] Periodic task is runnning\r\n",
                 (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
        vPrintString(cMsg);
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(30));

    }
}


/* 8 MHz HSE -> PLL x9 = 72 MHz SYSCLK/HCLK, APB1 = 36 MHz, APB2 = 72 MHz.
 * Stays on the 8 MHz HSI if the crystal or PLL fails to start. */
static void SystemClock_Config(void)
{
    uint32_t timeout;

    RCC->CR |= RCC_CR_HSEON;
    for (timeout = 100000; timeout && !(RCC->CR & RCC_CR_HSERDY); timeout--) {}
    if (!(RCC->CR & RCC_CR_HSERDY)) {
        RCC->CR &= ~RCC_CR_HSEON;
        return;
    }

    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;  /* 2 wait states for 48-72 MHz */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2 |
                               RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL))
              | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9;

    RCC->CR |= RCC_CR_PLLON;
    for (timeout = 100000; timeout && !(RCC->CR & RCC_CR_PLLRDY); timeout--) {}
    if (!(RCC->CR & RCC_CR_PLLRDY)) {
        RCC->CR &= ~RCC_CR_PLLON;
        return;
    }

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}
}

void vApplicationIdleHook(void)
{
    uldleCycleCount++;
}

int main(void)
{
    static const char *pcTextForTask1 = "Continous Task1 is running\r\n";
    static const char *pcTextForTask2 = "Continous Task2 is running\r\n";
    SystemInit();
    SystemClock_Config();  /* 72 MHz from 8 MHz HSE + PLL */
    SystemCoreClockUpdate();
    NVIC_SetPriorityGrouping(0);  /* all bits as preemption priority, required by FreeRTOS */
    vPrintInit();

    /* Create your tasks here, before starting the scheduler. */
    xTaskCreate(vTaskFunction, "Task1", 1000, (void*)pcTextForTask1, 1, NULL);
    xTaskCreate(vTaskFunction, "Task2", 1000, (void*)pcTextForTask2, 1, NULL);
    // xTaskCreate(vPeriodicTask, "vPeriodicTask", 1000, NULL, 2, NULL);
   
    vTaskStartScheduler();

    for (;;) {}
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t t, char *name)
{
    (void)t; (void)name;
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

void vApplicationTickHook(void)
{
    vTickTestHook();
}