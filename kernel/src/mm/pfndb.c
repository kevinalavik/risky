#include <boot/req.h>
#include <cpu.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <mm/page.h>
#include <mm/pfndb.h>

static page_t *g_pfndb;
static uint64_t g_pfndb_phys;
static uint64_t g_max_pfn;

static uint64_t pfndb_storage_size(void)
{
	return PAGE_ALIGN_UP((g_max_pfn + 1U) * sizeof(page_t));
}

static void pfndb_calc_max_pfn(const struct limine_memmap_response *memmap)
{
	g_max_pfn = 0;

	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		const struct limine_memmap_entry *entry = memmap->entries[i];
		uint64_t end_pfn;

		if (entry->type != LIMINE_MEMMAP_USABLE)
			continue;

		end_pfn = PAGE_ALIGN_UP(entry->base + entry->length) >> PAGE_SHIFT;
		if (end_pfn != 0 && end_pfn - 1U > g_max_pfn)
			g_max_pfn = end_pfn - 1U;
	}
}

static void pfndb_reserve_storage(struct limine_memmap_response *memmap)
{
	uint64_t size = pfndb_storage_size();

	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i];

		if (entry->type != LIMINE_MEMMAP_USABLE || entry->length < size)
			continue;

		g_pfndb_phys = entry->base;
		entry->base += size;
		entry->length -= size;
		return;
	}

	klog("pfndb: no usable memmap region large enough for db size=0x%llx\n",
		 (unsigned long long)size);
}

void pfndb_mark_range(uint64_t base, uint64_t length, uint32_t flags)
{
	uint64_t start_pfn = PHYS_PFN(base);
	uint64_t end_pfn = PHYS_PFN(PAGE_ALIGN_UP(base + length));

	if (g_pfndb == NULL || length == 0)
		return;

	if (start_pfn > g_max_pfn)
		return;

	if (end_pfn > g_max_pfn + 1U)
		end_pfn = g_max_pfn + 1U;

	for (uint64_t pfn = start_pfn; pfn < end_pfn; pfn++)
		page_set_state(&g_pfndb[pfn], flags, 0);
}

void pfndb_init(struct limine_memmap_response *memmap)
{
	if (memmap == NULL) {
		klog("pfndb: missing Limine memmap response\n");
		return;
	}

	if (hhdm_request.response == NULL) {
		klog("pfndb: missing Limine hhdm response\n");
		return;
	}

	pfndb_calc_max_pfn(memmap);
	pfndb_reserve_storage(memmap);
	if (g_pfndb_phys == 0) {
		klog("pfndb: failed to reserve pfndb storage\n");
		return;
	}

	g_pfndb = (page_t *)PHYS_TO_VIRT(g_pfndb_phys);
	memset(g_pfndb, 0, pfndb_storage_size());

	for (uint64_t pfn = 0; pfn <= g_max_pfn; pfn++)
		page_set_state(&g_pfndb[pfn], PAGE_RESERVED, 0);

	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i];

		if (entry->type == LIMINE_MEMMAP_USABLE)
			pfndb_mark_range(entry->base, entry->length, PAGE_FREE);
	}

	klog("pfndb: pages=%llu db_phys=0x%llx db_size=0x%llx\n",
		 (unsigned long long)(g_max_pfn + 1U),
		 (unsigned long long)g_pfndb_phys,
		 (unsigned long long)pfndb_storage_size());
}

page_t *pfndb_get_db(void)
{
	return g_pfndb;
}

uint64_t pfndb_get_max_pfn(void)
{
	return g_max_pfn;
}

page_t *pfndb_get_page(uint64_t pfn)
{
	if (g_pfndb == NULL || pfn > g_max_pfn)
		return NULL;

	return &g_pfndb[pfn];
}

uint64_t pfndb_get_pfn(const page_t *page)
{
	if (g_pfndb == NULL || page == NULL)
		return (uint64_t)-1;

	if (page < g_pfndb || page > &g_pfndb[g_max_pfn])
		return (uint64_t)-1;

	return (uint64_t)(page - g_pfndb);
}

uint64_t pfndb_page_to_phys(const page_t *page)
{
	uint64_t pfn = pfndb_get_pfn(page);

	if (pfn == (uint64_t)-1)
		return 0;

	return PFN_PHYS(pfn);
}

page_t *pfndb_phys_to_page(uint64_t phys)
{
	uint64_t pfn = PHYS_PFN(phys);

	if (pfn > g_max_pfn)
		return NULL;

	return pfndb_get_page(pfn);
}
