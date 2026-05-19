#ifndef MM_PFNDB_H
#define MM_PFNDB_H

#include <limine.h>
#include <stdint.h>
#include <mm/page.h>

#define page_to_pfn(page) pfndb_get_pfn(page)
#define pfn_to_page(pfn) pfndb_get_page(pfn)
#define page_to_phys(page) pfndb_page_to_phys(page)
#define phys_to_page(phys) pfndb_phys_to_page(phys)

void pfndb_init(struct limine_memmap_response *memmap);
void pfndb_mark_range(uint64_t base, uint64_t length, uint32_t flags);
page_t *pfndb_get_db(void);
uint64_t pfndb_get_max_pfn(void);
page_t *pfndb_get_page(uint64_t pfn);
uint64_t pfndb_get_pfn(const page_t *page);
uint64_t pfndb_page_to_phys(const page_t *page);
page_t *pfndb_phys_to_page(uint64_t phys);

#endif // MM_PFNDB_H
