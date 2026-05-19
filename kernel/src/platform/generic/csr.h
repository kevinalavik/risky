#ifndef PLATFORM_GENERIC_CSR_H
#define PLATFORM_GENERIC_CSR_H

#include <stdint.h>

#define csr_read(csr)                                       \
	({                                                      \
		uint64_t __value;                                   \
		__asm__ volatile("csrr %0, " #csr : "=r"(__value)); \
		__value;                                            \
	})

#define csr_write(csr, value)                                    \
	do {                                                         \
		uint64_t __value = (uint64_t)(value);                    \
		__asm__ volatile("csrw " #csr ", %0" : : "rK"(__value)); \
	} while (0)

#define csr_swap(csr, value)                      \
	({                                            \
		uint64_t __value = (uint64_t)(value);     \
		uint64_t __old;                           \
		__asm__ volatile("csrrw %0, " #csr ", %1" \
						 : "=r"(__old)            \
						 : "rK"(__value));        \
		__old;                                    \
	})

#define csr_read_set(csr, mask)                   \
	({                                            \
		uint64_t __mask = (uint64_t)(mask);       \
		uint64_t __old;                           \
		__asm__ volatile("csrrs %0, " #csr ", %1" \
						 : "=r"(__old)            \
						 : "rK"(__mask));         \
		__old;                                    \
	})

#define csr_set(csr, mask)                                      \
	do {                                                        \
		uint64_t __mask = (uint64_t)(mask);                     \
		__asm__ volatile("csrs " #csr ", %0" : : "rK"(__mask)); \
	} while (0)

#define csr_read_clear(csr, mask)                 \
	({                                            \
		uint64_t __mask = (uint64_t)(mask);       \
		uint64_t __old;                           \
		__asm__ volatile("csrrc %0, " #csr ", %1" \
						 : "=r"(__old)            \
						 : "rK"(__mask));         \
		__old;                                    \
	})

#define csr_clear(csr, mask)                                    \
	do {                                                        \
		uint64_t __mask = (uint64_t)(mask);                     \
		__asm__ volatile("csrc " #csr ", %0" : : "rK"(__mask)); \
	} while (0)

static inline void cpu_wfi(void)
{
	__asm__ volatile("wfi");
}

static inline void cpu_sfence_vma(uintptr_t vaddr)
{
	__asm__ volatile("sfence.vma %0, x0" : : "r"(vaddr) : "memory");
}

static inline void cpu_sfence_vma_all(void)
{
	__asm__ volatile("sfence.vma x0, x0" : : : "memory");
}

#endif // PLATFORM_GENERIC_CSR_H
