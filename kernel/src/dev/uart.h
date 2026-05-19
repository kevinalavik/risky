#ifndef DEV_UART_H
#define DEV_UART_H

#include <stdbool.h>
#include <stddef.h>

bool uart_init(void);
void uart_putc(char ch);
void uart_puts(const char *s);
char uart_getc(void);

#endif // DEV_UART_H
