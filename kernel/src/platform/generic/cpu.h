#ifndef PLATFORM_GENERIC_CPU_H
#define PLATFORM_GENERIC_CPU_H

#include <stdint.h>
#include <boot/req.h>
#include <csr.h>

#define PHYS_TO_VIRT(phys_addr) \
	((uintptr_t)(phys_addr) + (uintptr_t)hhdm_request.response->offset)

#define VIRT_TO_PHYS(virt_addr) \
	((uintptr_t)(virt_addr) - (uintptr_t)hhdm_request.response->offset)

static inline uint64_t cpu_read_time(void)
{
	return csr_read(time);
}

static inline void mmio_write8(uintptr_t phys_addr, uint8_t value)
{
	*(volatile uint8_t *)PHYS_TO_VIRT(phys_addr) = value;
}

static inline uint8_t mmio_read8(uintptr_t phys_addr)
{
	return *(volatile uint8_t *)PHYS_TO_VIRT(phys_addr);
}

static inline void mmio_write32(uintptr_t phys_addr, uint32_t value)
{
	*(volatile uint32_t *)PHYS_TO_VIRT(phys_addr) = value;
}

static inline uint32_t mmio_read32(uintptr_t phys_addr)
{
	return *(volatile uint32_t *)PHYS_TO_VIRT(phys_addr);
}

static inline void hlt(void)
{
	for (;;)
		__asm__ volatile("wfi");
}

#endif // PLATFORM_GENERIC_CPU_H
