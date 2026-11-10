#ifndef PRINT_H
#define PRINT_H

/* Configure USART1 (PA9 = TX, 115200 8N1) with DMA1 channel 4.
 * Call after the system clock is set up and before the scheduler starts. */
void vPrintInit(void);

/* Print a string over USART1 using DMA. Blocks only the calling task until the
 * transfer completes. Call from tasks only, not from interrupts. */
void vPrintString(const char *pcString);

#endif
