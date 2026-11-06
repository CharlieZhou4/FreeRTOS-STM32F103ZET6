#ifndef PRINT_H
#define PRINT_H

/* Print a string to the OpenOCD terminal via ARM semihosting (no UART needed).
 * Silently does nothing when no debugger is attached. */
void vPrintString(const char *pcString);

#endif
