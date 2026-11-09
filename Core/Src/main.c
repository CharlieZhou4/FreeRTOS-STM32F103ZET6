/* FreeRTOS learning skeleton for STM32F103RCT6: only brings up the clock and the scheduler. */
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

void vTaskFunction(void *pvParameters)
{
    char *pcTaskName;
    volatile uint32_t ul;
    pcTaskName = (char *)pvParameters;

    for (;;) 
    {
        vPrintString(pcTaskName);
        for (ul = 0; ul < 10000; ul++) 
        {
        }  /* crude delay */
    }
}


int main(void)
{
static const char *pcTextForTask1 = "Task1 is running\r\n";
static const char *pcTextForTask2 = "Task2 is running\r\n";
    SystemInit();  /* 72 MHz from 8 MHz HSE + PLL */
    SystemCoreClockUpdate();
    NVIC_SetPriorityGrouping(0);  /* all bits as preemption priority, required by FreeRTOS */

    /* Create your tasks here, before starting the scheduler. */
    //xTaskCreate(vTaskFunction, "Task1", 1000, (void *)pcTextForTask1, 1, NULL);
    //xTaskCreate(vTaskFunction, "Task2", 1000, (void *)pcTextForTask2, 1, NULL);
   
    vTickTestStart();

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
