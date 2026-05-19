#include <dev/dtb.h>
#include <dev/uart.h>
#include <cpu.h>
#include <lib/printk.h>
#include <mm/paging.h>
#include <mmio_defaults.h>

#define UART_RBR 0U
#define UART_THR 0U
#define UART_DLL 0U
#define UART_IER 1U
#define UART_DLM 1U
#define UART_FCR 2U
#define UART_LCR 3U
#define UART_LSR 5U

#define UART_LSR_RX_READY 0x01U
#define UART_LSR_TX_EMPTY 0x20U
#define UART_LCR_DLAB 0x80U

#define UART_PATH_MAX 256U

static const char *const uart_compatibles[] = {
	"ns16550a",
	"ns16550",
	"snps,dw-apb-uart",
	"uart8250",
};

struct uart_port {
	uintptr_t base;
	uint64_t size;
	uint32_t reg_shift;
	uint32_t reg_io_width;
};

static struct uart_port uart0;
static bool uart_ready;

bool uart_is_ready(void)
{
	return uart_ready;
}

static void uart_write_reg(uint32_t reg, uint8_t val)
{
	uintptr_t addr = uart0.base + ((uintptr_t)reg << uart0.reg_shift);

	if (uart0.reg_io_width == 4U)
		mmio_write32(addr, val);
	else
		mmio_write8(addr, val);
}

static uint8_t uart_read_reg(uint32_t reg)
{
	uintptr_t addr = uart0.base + ((uintptr_t)reg << uart0.reg_shift);

	if (uart0.reg_io_width == 4U)
		return (uint8_t)mmio_read32(addr);
	return mmio_read8(addr);
}

static bool uart_find_port(struct uart_port *port)
{
	char path[UART_PATH_MAX];
	char resolved[UART_PATH_MAX];
	struct dtb_node node;
	uint64_t size;
	size_t i;
	bool found = false;

	if (dtb_get_stdout_path(path, sizeof(path))) {
		const char *p = path;

		if (path[0] != '/') {
			if (dtb_resolve_alias(path, resolved, sizeof(resolved)))
				p = resolved;
		}

		if (dtb_find_node_by_path(p, &node)) {
			for (i = 0;
				 i < sizeof(uart_compatibles) / sizeof(uart_compatibles[0]);
				 i++) {
				if (dtb_node_has_compatible(&node, uart_compatibles[i])) {
					found = true;
					break;
				}
			}
		}
	}

	if (!found) {
		for (i = 0; i < sizeof(uart_compatibles) / sizeof(uart_compatibles[0]);
			 i++) {
			if (dtb_find_compatible(uart_compatibles[i], &node)) {
				found = true;
				break;
			}
		}
	}

	if (!found)
		klog("uart: failed to find uart in dtb, using fallback\n");
	if (!found)
		return false;

	if (!dtb_get_reg(&node, 0, &port->base, &size) || size == 0) {
		klog("uart: failed to read uart reg from dtb\n");
		return false;
	}

	port->size = size;
	port->reg_shift = 0;
	port->reg_io_width = 1;

	(void)dtb_get_u32(&node, "reg-shift", &port->reg_shift);
	if (dtb_get_u32(&node, "reg-io-width", &port->reg_io_width) &&
		port->reg_io_width != 1U && port->reg_io_width != 4U) {
		klog("uart: unsupported reg-io-width=%u\n", port->reg_io_width);
		return false;
	}

	return true;
}

bool uart_init(void)
{
	uart_ready = false;

	if (!uart_find_port(&uart0)) {
		uart0.base = UART_DEFAULT_BASE;
		uart0.size = UART_DEFAULT_SIZE;
		uart0.reg_shift = 0;
		uart0.reg_io_width = 1;
		klog("uart: using default fallback uart at phys=0x%llx\n",
			 (unsigned long long)uart0.base);
	}

	if (!paging_map_mmio(uart0.base, uart0.size)) {
		klog("uart: failed to map mmio phys=0x%llx size=0x%llx\n",
			 (unsigned long long)uart0.base, (unsigned long long)uart0.size);
		return false;
	}

	uart_write_reg(UART_IER, 0x00);
	uart_write_reg(UART_LCR, UART_LCR_DLAB);
	uart_write_reg(UART_DLL, 0x02);
	uart_write_reg(UART_DLM, 0x00);
	uart_write_reg(UART_LCR, 0x03);
	uart_write_reg(UART_FCR, 0x07);
	uart_ready = true;
	klog("uart: init ok phys=0x%llx shift=%u width=%u\n",
		 (unsigned long long)uart0.base, uart0.reg_shift, uart0.reg_io_width);

	while ((uart_read_reg(UART_LSR) & UART_LSR_RX_READY) != 0U)
		(void)uart_read_reg(UART_RBR);

	return true;
}

void uart_putc(char c)
{
	if (!uart_ready)
		return;

	for (unsigned long spins = 0;
		 spins < 1000000UL &&
		 (uart_read_reg(UART_LSR) & UART_LSR_TX_EMPTY) == 0U;
		 spins++) {
	}

	uart_write_reg(UART_THR, (uint8_t)c);
}

void uart_puts(const char *s)
{
	while (*s != '\0')
		uart_putc(*s++);
}

char uart_getc(void)
{
	if (!uart_ready)
		return '\0';

	while ((uart_read_reg(UART_LSR) & UART_LSR_RX_READY) == 0U)
		;

	return (char)uart_read_reg(UART_RBR);
}
