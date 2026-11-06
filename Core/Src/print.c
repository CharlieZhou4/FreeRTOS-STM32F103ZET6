#include "stm32f1xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "print.h"

#define SEMIHOST_SYS_WRITE0 0x04

static void semihost_call(uint32_t op, const void *arg)
{
    register uint32_t r0 __asm__("r0") = op;
    register const void *r1 __asm__("r1") = arg;
    __asm__ volatile("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
}

void vPrintString(const char *pcString)
{
    /* bkpt without a debugger would raise HardFault, so skip it in that case */
    if (!(CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)) {
        return;
    }
    taskENTER_CRITICAL();
    semihost_call(SEMIHOST_SYS_WRITE0, pcString);
    taskEXIT_CRITICAL();
}
