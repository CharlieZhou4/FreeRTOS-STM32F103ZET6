#ifndef TICK_TEST_H
#define TICK_TEST_H

/* Create a task that verifies SysTick period and vTaskDelay timing using the DWT cycle counter.
 * Results are printed through vPrintString once per second. */
void vTickTestStart(void);

/* Call from vApplicationTickHook (requires configUSE_TICK_HOOK = 1). */
void vTickTestHook(void);

#endif
