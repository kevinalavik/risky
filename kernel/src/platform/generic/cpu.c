#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <cpu.h>

#define SATP_MODE_SV39 8UL
#define SATP_MODE_SV48 9UL
#define SATP_MODE_SV57 10UL

#define PAGE_SIZE 4096UL
#define PAGE_MASK (~(PAGE_SIZE - 1UL))
#define PT_ENTRIES 512UL

#define PTE_V (1UL << 0)
#define PTE_R (1UL << 1)
#define PTE_W (1UL << 2)
#define PTE_X (1UL << 3)
#define PTE_A (1UL << 6)
#define PTE_D (1UL << 7)

#define PTE_LEAF_MASK (PTE_R | PTE_W | PTE_X)
#define MMIO_PTE_FLAGS (PTE_V | PTE_R | PTE_W | PTE_A | PTE_D)

#define MMIO_PT_POOL_PAGES 8U

static uintptr_t cpu_hhdm_offset;
static uintptr_t cpu_kernel_phys_base;
static uintptr_t cpu_kernel_virt_base;
static uint64_t mmio_pt_pool[MMIO_PT_POOL_PAGES][PT_ENTRIES]
	__attribute__((aligned(PAGE_SIZE)));
static size_t mmio_pt_pool_used;

static inline uint64_t cpu_read_satp(void)
{
	uint64_t satp;

	__asm__ volatile("csrr %0, satp" : "=r"(satp));
	return satp;
}

static inline void cpu_sfence_vma(uintptr_t vaddr)
{
	__asm__ volatile("sfence.vma %0, x0" : : "r"(vaddr) : "memory");
}

static unsigned cpu_satp_levels(uint64_t satp)
{
	switch (satp >> 60) {
	case SATP_MODE_SV39:
		return 3;
	case SATP_MODE_SV48:
		return 4;
	case SATP_MODE_SV57:
		return 5;
	default:
		return 0;
	}
}

static uintptr_t cpu_kernel_virt_to_phys(uintptr_t virt_addr)
{
	return virt_addr - cpu_kernel_virt_base + cpu_kernel_phys_base;
}

static uintptr_t cpu_pte_to_phys(uint64_t pte)
{
	return ((pte >> 10) << 12);
}

static uint64_t cpu_phys_to_table_pte(uintptr_t phys_addr)
{
	return ((uint64_t)(phys_addr >> 12) << 10) | PTE_V;
}

static uint64_t cpu_phys_to_leaf_pte(uintptr_t phys_addr)
{
	return ((uint64_t)(phys_addr >> 12) << 10) | MMIO_PTE_FLAGS;
}

static uint64_t *cpu_alloc_pt_page(uintptr_t *phys_addr)
{
	uint64_t *page;

	if (mmio_pt_pool_used >= MMIO_PT_POOL_PAGES) {
		return NULL;
	}

	page = mmio_pt_pool[mmio_pt_pool_used++];
	for (size_t i = 0; i < PT_ENTRIES; i++) {
		page[i] = 0;
	}

	*phys_addr = cpu_kernel_virt_to_phys((uintptr_t)page);
	return page;
}

void cpu_init_mappings(uint64_t hhdm_offset, uint64_t kernel_phys_base,
					   uint64_t kernel_virt_base)
{
	cpu_hhdm_offset = (uintptr_t)hhdm_offset;
	cpu_kernel_phys_base = (uintptr_t)kernel_phys_base;
	cpu_kernel_virt_base = (uintptr_t)kernel_virt_base;
	mmio_pt_pool_used = 0;
}

uintptr_t phys_to_virt(uintptr_t phys_addr)
{
	return phys_addr + cpu_hhdm_offset;
}

bool cpu_map_mmio(uintptr_t phys_addr, uint64_t size)
{
	uint64_t satp = cpu_read_satp();
	unsigned levels = cpu_satp_levels(satp);
	uintptr_t root_phys = (satp & ((1ULL << 44) - 1ULL)) << 12;
	uintptr_t start_phys = phys_addr & PAGE_MASK;
	uintptr_t end_phys = (phys_addr + size + PAGE_SIZE - 1UL) & PAGE_MASK;

	if (levels == 0 || root_phys == 0 || size == 0) {
		return false;
	}

	for (uintptr_t page_phys = start_phys; page_phys < end_phys;
		 page_phys += PAGE_SIZE) {
		uintptr_t virt_addr = phys_to_virt(page_phys);
		uint64_t *table = (uint64_t *)phys_to_virt(root_phys);

		for (unsigned level = levels - 1; level > 0; level--) {
			uintptr_t index = (virt_addr >> (12 + level * 9)) & 0x1ffUL;
			uint64_t pte = table[index];

			if ((pte & PTE_V) == 0) {
				uintptr_t new_phys;
				uint64_t *new_table = cpu_alloc_pt_page(&new_phys);

				if (new_table == NULL) {
					return false;
				}

				table[index] = cpu_phys_to_table_pte(new_phys);
				pte = table[index];
			} else if ((pte & PTE_LEAF_MASK) != 0) {
				table = NULL;
				break;
			}

			table = (uint64_t *)phys_to_virt(cpu_pte_to_phys(pte));
		}

		if (table != NULL) {
			uintptr_t index = (virt_addr >> 12) & 0x1ffUL;
			table[index] = cpu_phys_to_leaf_pte(page_phys);
		}

		cpu_sfence_vma(virt_addr);
	}

	return true;
}
