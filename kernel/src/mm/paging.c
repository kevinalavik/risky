#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <boot/req.h>
#include <cpu.h>
#include <csr.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <mm/page.h>
#include <mm/paging.h>
#include <mm/pfndb.h>
#include <mm/pmm.h>

extern char __kernel_start[];
extern char __kernel_end[];
extern char __limine_requests_start[];
extern char __text_start[];
extern char __text_end[];
extern char __rodata_start[];
extern char __rodata_end[];
extern char __data_start[];
extern char __bss_end[];

#define SATP_MODE_SV39 8UL
#define SATP_MODE_SV48 9UL
#define SATP_MODE_SV57 10UL
#define SATP_PPN_MASK ((1ULL << 44) - 1ULL)

#define PT_ENTRIES 512UL

#define PTE_V (1UL << 0)
#define PTE_R (1UL << 1)
#define PTE_W (1UL << 2)
#define PTE_X (1UL << 3)
#define PTE_U (1UL << 4)
#define PTE_G (1UL << 5)
#define PTE_A (1UL << 6)
#define PTE_D (1UL << 7)

#define PTE_TABLE_FLAGS PTE_V

#define PAGING_PROT_RW (PAGING_FLAG_READ | PAGING_FLAG_WRITE)
#define PAGING_PROT_RX (PAGING_FLAG_READ | PAGING_FLAG_EXEC)
#define PAGING_PROT_RWG (PAGING_PROT_RW | PAGING_FLAG_GLOBAL)
#define PAGING_PROT_RXG (PAGING_PROT_RX | PAGING_FLAG_GLOBAL)
#define PAGING_PROT_RG (PAGING_FLAG_READ | PAGING_FLAG_GLOBAL)

#define PAGING_KERNEL_SEGMENT_LIST(X)                           \
	X("limine requests", __limine_requests_start, __text_start, \
	  PAGING_PROT_RWG)                                          \
	X("text", __text_start, __text_end, PAGING_PROT_RXG)        \
	X("rodata", __rodata_start, __rodata_end, PAGING_PROT_RG)   \
	X("data/bss", __data_start, __bss_end, PAGING_PROT_RWG)

static uintptr_t g_root_phys;
static unsigned g_levels;
static uint64_t g_early_pt_pool[8][PT_ENTRIES]
	__attribute__((aligned(PAGE_SIZE)));
static size_t g_early_pt_pool_used;

static unsigned paging_satp_levels(uint64_t satp);
static inline uintptr_t paging_level_shift(unsigned level);
static inline uintptr_t paging_level_block_size(unsigned level);
static inline uintptr_t paging_level_index(uintptr_t virt_addr, unsigned level);

static bool paging_setup_root(void)
{
	uint64_t satp = csr_read(satp);

	g_levels = paging_satp_levels(satp);
	g_root_phys = (uintptr_t)((satp & SATP_PPN_MASK) << 12);
	return g_levels != 0 && g_root_phys != 0;
}

static unsigned paging_satp_levels(uint64_t satp)
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

static uint64_t paging_leaf_flags(uint64_t flags)
{
	uint64_t pte = PTE_V | PTE_A;

	if ((flags & PAGING_FLAG_READ) != 0)
		pte |= PTE_R;

	if ((flags & PAGING_FLAG_WRITE) != 0)
		pte |= PTE_R | PTE_W | PTE_D;

	if ((flags & PAGING_FLAG_EXEC) != 0)
		pte |= PTE_X;

	if ((flags & PAGING_FLAG_GLOBAL) != 0)
		pte |= PTE_G;

	return pte;
}

static uintptr_t paging_pte_to_phys(uint64_t pte)
{
	return (uintptr_t)((pte >> 10) << 12);
}

static uint64_t paging_table_pte(uintptr_t phys_addr)
{
	return ((uint64_t)(phys_addr >> 12) << 10) | PTE_TABLE_FLAGS;
}

static bool paging_entry_covers(uint64_t entry, uintptr_t virt_addr,
								uintptr_t phys_addr, uint64_t flags,
								unsigned level)
{
	uintptr_t block_size = paging_level_block_size(level);
	uintptr_t block_mask = block_size - 1UL;
	uintptr_t mapped_phys =
		paging_pte_to_phys(entry) + (virt_addr & block_mask);
	uint64_t want = paging_leaf_flags(flags) & (PTE_R | PTE_W | PTE_X);

	if ((entry & PTE_V) == 0)
		return false;

	if (mapped_phys != phys_addr)
		return false;

	if ((entry & want) != want)
		return false;

	if ((flags & PAGING_FLAG_WRITE) != 0 && (entry & PTE_D) == 0)
		return false;

	if ((entry & PTE_A) == 0) {
		return false;
	}

	return true;
}

static uint64_t *paging_root_virt(void)
{
	return (uint64_t *)PHYS_TO_VIRT(g_root_phys);
}

static inline uintptr_t paging_level_shift(unsigned level)
{
	return 12U + level * 9U;
}

static inline uintptr_t paging_level_block_size(unsigned level)
{
	return 1UL << paging_level_shift(level);
}

static inline uintptr_t paging_level_index(uintptr_t virt_addr, unsigned level)
{
	return (virt_addr >> paging_level_shift(level)) & 0x1ffUL;
}

static uint64_t *paging_alloc_table(void)
{
	uintptr_t phys_addr;
	uint64_t *table;
	page_t *page;

	page = pmm_is_ready() ? page_alloc(0) : NULL;
	if (page != NULL) {
		phys_addr = page_to_phys(page);
		table = (uint64_t *)PHYS_TO_VIRT(phys_addr);
		memset(table, 0, PAGE_SIZE);
		return table;
	}

	if (g_early_pt_pool_used >=
		sizeof(g_early_pt_pool) / sizeof(g_early_pt_pool[0])) {
		klog("paging: failed to allocate page-table page");
		return NULL;
	}

	table = g_early_pt_pool[g_early_pt_pool_used++];
	phys_addr = VIRT_TO_PHYS((uintptr_t)table);
	memset(table, 0, PAGE_SIZE);
	return table;
}

static bool paging_map_one(uint64_t *root, uintptr_t virt_addr,
						   uintptr_t phys_addr, uint64_t flags)
{
	uint64_t *table = root;

	for (unsigned level = g_levels - 1; level > 0; level--) {
		uintptr_t index = paging_level_index(virt_addr, level);
		uint64_t entry = table[index];

		if ((entry & PTE_V) == 0) {
			uint64_t *new_table = paging_alloc_table();
			uintptr_t new_phys;

			if (new_table == NULL)
				return false;

			new_phys = VIRT_TO_PHYS((uintptr_t)new_table);
			table[index] = paging_table_pte(new_phys);
			entry = table[index];
		}

		if ((entry & (PTE_R | PTE_W | PTE_X)) != 0) {
			if (paging_entry_covers(entry, virt_addr, phys_addr, flags, level))
				return true;

			klog(
				"paging: conflicting leaf virt=0x%llx phys=0x%llx level=%u entry=0x%llx",
				(unsigned long long)virt_addr, (unsigned long long)phys_addr,
				level, (unsigned long long)entry);
			return false;
		}

		table = (uint64_t *)PHYS_TO_VIRT(paging_pte_to_phys(entry));
	}

	uintptr_t page_index = paging_level_index(virt_addr, 0);

	if ((table[page_index] & PTE_V) != 0) {
		uint64_t entry = table[page_index];

		if (paging_entry_covers(entry, virt_addr, phys_addr, flags, 0))
			return true;
	}

	table[page_index] =
		((uint64_t)(phys_addr >> PAGE_SHIFT) << 10) | paging_leaf_flags(flags);
	cpu_sfence_vma(virt_addr);
	return true;
}

static bool paging_map_kernel_segment(uintptr_t virt_start, uintptr_t virt_end,
									  uint64_t flags)
{
	uintptr_t start = (uintptr_t)PAGE_ALIGN_DOWN(virt_start);
	uintptr_t end = (uintptr_t)PAGE_ALIGN_UP(virt_end);
	uintptr_t phys;
	uintptr_t virt;
	uintptr_t kernel_phys_base;
	uintptr_t kernel_virt_base;

	if (start >= end || executable_address_request.response == NULL)
		return true;

	kernel_phys_base =
		(uintptr_t)executable_address_request.response->physical_base;
	kernel_virt_base =
		(uintptr_t)executable_address_request.response->virtual_base;

	for (virt = start; virt < end; virt += PAGE_SIZE) {
		phys = kernel_phys_base + (virt - kernel_virt_base);
		if (!paging_map_page(virt, phys, flags))
			return false;
	}

	return true;
}

static bool paging_map_hhdm(void)
{
	struct limine_memmap_response *memmap = memmap_request.response;

	if (memmap == NULL || hhdm_request.response == NULL) {
		klog("paging: missing memmap or hhdm response for hhdm map");
		return false;
	}

	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i];
		uintptr_t phys_start = (uintptr_t)PAGE_ALIGN_DOWN(entry->base);
		uintptr_t phys_end =
			(uintptr_t)PAGE_ALIGN_UP(entry->base + entry->length);

		if (phys_start == phys_end)
			continue;

		if (!paging_map_range((uintptr_t)PHYS_TO_VIRT(phys_start), phys_start,
							  phys_end - phys_start, PAGING_PROT_RWG)) {
			klog("paging: failed to map hhdm phys=[0x%llx,0x%llx)",
				 (unsigned long long)phys_start, (unsigned long long)phys_end);
			return false;
		}
	}

	return true;
}

static bool paging_map_kernel_segments(void)
{
	struct paging_segment {
		const char *name;
		uintptr_t start;
		uintptr_t end;
		uint64_t flags;
	};

	static const struct paging_segment segments[] = {
#define PAGING_SEGMENT_ENTRY(name, start, end, flags) \
	{ (name), (uintptr_t)(start), (uintptr_t)(end), (flags) },
		PAGING_KERNEL_SEGMENT_LIST(PAGING_SEGMENT_ENTRY)
#undef PAGING_SEGMENT_ENTRY
	};

	for (size_t i = 0; i < sizeof(segments) / sizeof(segments[0]); i++) {
		if (!paging_map_kernel_segment(segments[i].start, segments[i].end,
									   segments[i].flags)) {
			klog("paging: failed to map %s segment", segments[i].name);
			return false;
		}
	}

	return true;
}

bool paging_early_init(void)
{
	g_early_pt_pool_used = 0;

	if (!paging_setup_root()) {
		klog("paging: early init failed to setup root");
		return false;
	}

	if (hhdm_request.response == NULL) {
		klog("paging: missing hhdm response for early init");
		return false;
	}

	return true;
}

bool paging_init(void)
{
	g_early_pt_pool_used = 0;

	if (!paging_setup_root() || hhdm_request.response == NULL ||
		memmap_request.response == NULL ||
		executable_address_request.response == NULL) {
		uint64_t satp = csr_read(satp);
		klog(
			"paging: init failed satp=0x%llx levels=%u root=0x%llx hhdm=%p memmap=%p exec=%p",
			(unsigned long long)satp, g_levels, (unsigned long long)g_root_phys,
			hhdm_request.response, memmap_request.response,
			executable_address_request.response);
		return false;
	}

	if (!paging_map_hhdm())
		return false;

	if (!paging_map_kernel_segments()) {
		return false;
	}

	cpu_sfence_vma_all();
	klog("paging: init ok root=0x%llx levels=%u",
		 (unsigned long long)g_root_phys, g_levels);
	return true;
}

bool paging_map_page(uintptr_t virt_addr, uintptr_t phys_addr, uint64_t flags)
{
	if ((virt_addr & (PAGE_SIZE - 1UL)) != 0 ||
		(phys_addr & (PAGE_SIZE - 1UL)) != 0 || g_root_phys == 0) {
		klog("paging: bad page map virt=0x%llx phys=0x%llx root=0x%llx",
			 (unsigned long long)virt_addr, (unsigned long long)phys_addr,
			 (unsigned long long)g_root_phys);
		return false;
	}

	return paging_map_one(paging_root_virt(), virt_addr, phys_addr, flags);
}

bool paging_map_range(uintptr_t virt_addr, uintptr_t phys_addr, uint64_t size,
					  uint64_t flags)
{
	uintptr_t end;

	if (size == 0)
		return true;

	end = virt_addr + (uintptr_t)PAGE_ALIGN_UP(size);
	while (virt_addr < end) {
		if (!paging_map_page(virt_addr, phys_addr, flags))
			return false;

		virt_addr += PAGE_SIZE;
		phys_addr += PAGE_SIZE;
	}

	return true;
}

bool paging_map_mmio(uintptr_t phys_addr, uint64_t size)
{
	uintptr_t start = (uintptr_t)PAGE_ALIGN_DOWN(phys_addr);
	uintptr_t end = (uintptr_t)PAGE_ALIGN_UP(phys_addr + size);

	if (size == 0) {
		klog("paging: refusing zero-sized mmio map for phys=0x%llx",
			 (unsigned long long)phys_addr);
		return false;
	}

	return paging_map_range((uintptr_t)PHYS_TO_VIRT(start), start, end - start,
							PAGING_PROT_RWG);
}

uintptr_t paging_root_phys(void)
{
	return g_root_phys;
}
