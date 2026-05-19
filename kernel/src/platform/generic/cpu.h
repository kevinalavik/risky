#ifndef PLATFORM_GENERIC_CPU_H
#define PLATFORM_GENERIC_CPU_H

#include <stdint.h>

void cpu_init_mappings(uint64_t hhdm_offset, uint64_t kernel_phys_base,
					   uint64_t kernel_virt_base);
uintptr_t phys_to_virt(uintptr_t phys_addr);
bool cpu_map_mmio(uintptr_t phys_addr, uint64_t size);

static inline void mmio_write8(uintptr_t phys_addr, uint8_t value)
{
	*(volatile uint8_t *)phys_to_virt(phys_addr) = value;
}

static inline uint8_t mmio_read8(uintptr_t phys_addr)
{
	return *(volatile uint8_t *)phys_to_virt(phys_addr);
}

static inline void mmio_write32(uintptr_t phys_addr, uint32_t value)
{
	*(volatile uint32_t *)phys_to_virt(phys_addr) = value;
}

static inline uint32_t mmio_read32(uintptr_t phys_addr)
{
	return *(volatile uint32_t *)phys_to_virt(phys_addr);
}

static inline void hlt(void)
{
	for (;;)
		__asm__ volatile("wfi");
}

#endif // PLATFORM_GENERIC_CPU_H
