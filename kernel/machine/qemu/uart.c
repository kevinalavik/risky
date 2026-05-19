#include <arch/mmio.h>
#include <machine/uart.h>
#include <sys/types.h>

#define UART0_BASE 0x10000000UL
#define UART_THR 0x00
#define UART_LSR 0x05
#define UART_LSR_THRE (1u << 5)

static volatile uint8_t *const uart0 = (volatile uint8_t *)UART0_BASE;

void uart_init(void) {}

void uart_putchar(char ch) {
  while ((mmio_read8((uint64_t)&uart0[UART_LSR]) & UART_LSR_THRE) == 0) {
  }

  mmio_write8((uint64_t)&uart0[UART_THR], (uint8_t)ch);
}
