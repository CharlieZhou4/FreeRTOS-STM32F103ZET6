#include <stdio.h>
#include "stm32f1xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "print.h"
#include "tick_test.h"

#define TEST_DELAY_MS 1000

static volatile uint32_t ulHookTicks;
static volatile uint32_t ulLastCycles;
static volatile uint32_t ulMinPeriod = UINT32_MAX;
static volatile uint32_t ulMaxPeriod;
static volatile uint32_t ulBadTicks;

void vTickTestHook(void)
{
    uint32_t now = DWT->CYCCNT;
    uint32_t period = now - ulLastCycles;
    uint32_t expected = SystemCoreClock / configTICK_RATE_HZ;

    ulLastCycles = now;
    if (ulHookTicks++ == 0) {
        return;  /* first call has no previous tick */
    }
    if (period < ulMinPeriod) ulMinPeriod = period;
    if (period > ulMaxPeriod) ulMaxPeriod = period;
    /* a tick may be delayed by critical sections, tolerate 1% */
    if (period > expected + expected / 100 || period + expected / 100 < expected) {
        ulBadTicks++;
    }
}

static void vTickTestTask(void *pvParameters)
{
    char buf[128];
    (void)pvParameters;

    for (;;) {
        TickType_t tick0 = xTaskGetTickCount();
        uint32_t cyc0 = DWT->CYCCNT;
        vTaskDelay(pdMS_TO_TICKS(TEST_DELAY_MS));
        uint32_t cycles = DWT->CYCCNT - cyc0;
        TickType_t ticks = xTaskGetTickCount() - tick0;

        uint32_t us = cycles / (SystemCoreClock / 1000000U);
        uint32_t expectedCycles = (SystemCoreClock / 1000U) * TEST_DELAY_MS;
        int32_t errPpm = (int32_t)(((int64_t)cycles - expectedCycles) * 1000000 / expectedCycles);

        snprintf(buf, sizeof buf,
                 "[tick] delay: %lu ticks, %lu us, err %ld ppm | period cycles min %lu max %lu (exp %lu) bad %lu\r\n",
                 (unsigned long)ticks, (unsigned long)us, (long)errPpm,
                 (unsigned long)ulMinPeriod, (unsigned long)ulMaxPeriod,
                 (unsigned long)(SystemCoreClock / configTICK_RATE_HZ), (unsigned long)ulBadTicks);
        vPrintString(buf);
    }
}

/* Demo: the same "1000" written as a raw tick count vs. pdMS_TO_TICKS(1000) */
static void vHardcodedDelayTask(void *pvParameters)
{
    char buf[96];
    (void)pvParameters;

    for (;;) {
        uint32_t cyc0 = DWT->CYCCNT;
        vTaskDelay(1000);
        uint32_t ms = (DWT->CYCCNT - cyc0) / (SystemCoreClock / 1000U);
        snprintf(buf, sizeof buf, "[demo] vTaskDelay(1000)              -> real wait %lu ms\r\n", (unsigned long)ms);
        vPrintString(buf);
    }
}

static void vMacroDelayTask(void *pvParameters)
{
    char buf[96];
    (void)pvParameters;

    for (;;) {
        uint32_t cyc0 = DWT->CYCCNT;
        vTaskDelay(pdMS_TO_TICKS(1000));
        uint32_t ms = (DWT->CYCCNT - cyc0) / (SystemCoreClock / 1000U);
        snprintf(buf, sizeof buf, "[demo] vTaskDelay(pdMS_TO_TICKS(1000)) -> real wait %lu ms\r\n", (unsigned long)ms);
        vPrintString(buf);
    }
}

void vTickTestStart(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    xTaskCreate(vTickTestTask, "TickTest", 512, NULL, 2, NULL);
    xTaskCreate(vHardcodedDelayTask, "Hard", 512, NULL, 2, NULL);
    xTaskCreate(vMacroDelayTask, "Macro", 512, NULL, 2, NULL);
}
