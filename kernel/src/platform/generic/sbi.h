#ifndef PLATFORM_GENERIC_SBI_H
#define PLATFORM_GENERIC_SBI_H

#include <stdint.h>

static inline void sbi_legacy_console_putchar(int ch)
{
	register uintptr_t a0 __asm__("a0") = (uintptr_t)ch;
	register uintptr_t a7 __asm__("a7") = 0x1;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a7) : "memory");
}

#endif // PLATFORM_GENERIC_SBI_H
