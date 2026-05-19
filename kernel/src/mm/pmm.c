#include <lib/string.h>
#include <lib/printk.h>
#include <mm/page.h>
#include <mm/pfndb.h>
#include <mm/pmm.h>

struct pmm_free_list {
	page_t *head;
	uint64_t count;
};

static struct pmm_free_list g_free_lists[PMM_MAX_ORDER];
static uint64_t g_free_pages;
static uint64_t g_total_pages;
static bool g_pmm_ready;

static void pmm_prepare_page(page_t *page, uint32_t flags, uint8_t order)
{
	page_set_state(page, flags, order);
}

static void pmm_list_push(uint8_t order, page_t *page)
{
	page_reset_links(page);
	page->next = g_free_lists[order].head;
	if (g_free_lists[order].head != NULL)
		g_free_lists[order].head->prev = page;

	g_free_lists[order].head = page;
	g_free_lists[order].count++;
}

static void pmm_list_remove(uint8_t order, page_t *page)
{
	if (page->prev != NULL)
		page->prev->next = page->next;
	else
		g_free_lists[order].head = page->next;

	if (page->next != NULL)
		page->next->prev = page->prev;

	page_reset_links(page);
	g_free_lists[order].count--;
}

static uint64_t pmm_buddy_pfn(uint64_t pfn, uint8_t order)
{
	return pfn ^ (1ULL << order);
}

static void pmm_free_block(page_t *page)
{
	uint64_t pfn = page_to_pfn(page);
	uint8_t order = page->order;

	while (order + 1U < PMM_MAX_ORDER) {
		uint64_t buddy_pfn = pmm_buddy_pfn(pfn, order);
		page_t *buddy;

		if (buddy_pfn > pfndb_get_max_pfn())
			break;

		buddy = pfn_to_page(buddy_pfn);
		if (buddy == NULL || !PageFree(buddy) || buddy->order != order)
			break;

		pmm_list_remove(order, buddy);
		if (buddy_pfn < pfn) {
			pfn = buddy_pfn;
			page = buddy;
		}

		order++;
		page->order = order;
	}

	pmm_prepare_page(page, PAGE_FREE, order);
	pmm_list_push(order, page);
}

bool pmm_init(void)
{
	page_t *page;

	if (pfndb_get_db() == NULL) {
		klog("pmm: pfndb is not initialized");
		return false;
	}

	memset(g_free_lists, 0, sizeof(g_free_lists));
	g_free_pages = 0;
	g_total_pages = 0;
	g_pmm_ready = false;

	for (uint64_t pfn = 0; pfn <= pfndb_get_max_pfn(); pfn++) {
		page = pfn_to_page(pfn);
		if (page == NULL)
			continue;

		page->order = 0;
		page_reset_links(page);

		if (PageFree(page)) {
			SetPageUsed(page);
			ClearPageFree(page);
			g_total_pages++;
		}
	}

	for (uint64_t pfn = 0; pfn <= pfndb_get_max_pfn(); pfn++) {
		page = pfn_to_page(pfn);
		if (page == NULL || !PageUsed(page) || PageReserved(page))
			continue;

		g_free_pages++;
		pmm_free_block(page);
	}

	klog("pmm: total=%llu free=%llu", (unsigned long long)g_total_pages,
		 (unsigned long long)g_free_pages);
	g_pmm_ready = true;

	return true;
}

bool pmm_is_ready(void)
{
	return g_pmm_ready;
}

page_t *pmm_alloc(uint8_t order)
{
	if (!g_pmm_ready)
		return NULL;

	if (order >= PMM_MAX_ORDER) {
		klog("pmm: alloc order %u out of range", order);
		return NULL;
	}

	for (uint8_t current = order; current < PMM_MAX_ORDER; current++) {
		page_t *page = g_free_lists[current].head;

		if (page == NULL)
			continue;

		pmm_list_remove(current, page);
		while (current > order) {
			page_t *buddy;

			current--;
			buddy = pfn_to_page(page_to_pfn(page) + (1ULL << current));
			pmm_prepare_page(buddy, PAGE_FREE, current);
			pmm_list_push(current, buddy);
		}

		pmm_prepare_page(page, PAGE_USED, order);
		g_free_pages -= 1ULL << order;
		return page;
	}

	klog("pmm: alloc failed for order %u", order);
	return NULL;
}

page_t *pmm_alloc_page(void)
{
	return pmm_alloc(0);
}

void pmm_free(page_t *page)
{
	if (!g_pmm_ready) {
		klog("pmm: refusing free before init");
		return;
	}

	if (page == NULL) {
		klog("pmm: ignoring free of NULL page");
		return;
	}

	if (PageReserved(page)) {
		klog("pmm: refusing to free reserved page pfn=0x%llx",
			 (unsigned long long)page_to_pfn(page));
		return;
	}

	if (PageFree(page)) {
		klog("pmm: refusing double free of pfn=0x%llx",
			 (unsigned long long)page_to_pfn(page));
		return;
	}

	g_free_pages += 1ULL << page->order;
	pmm_free_block(page);
}

uint64_t pmm_free_pages(void)
{
	return g_free_pages;
}

uint64_t pmm_total_pages(void)
{
	return g_total_pages;
}
