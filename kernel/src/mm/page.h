#ifndef MM_PAGE_H
#define MM_PAGE_H

#include <stdbool.h>
#include <stdint.h>

#define PAGE_SHIFT 12UL
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1UL))

#define PAGE_ALIGN_DOWN(addr) ((uint64_t)(addr) & PAGE_MASK)
#define PAGE_ALIGN_UP(addr) (((uint64_t)(addr) + PAGE_SIZE - 1UL) & PAGE_MASK)

#define PHYS_PFN(addr) ((uint64_t)(addr) >> PAGE_SHIFT)
#define PFN_PHYS(pfn) ((uint64_t)(pfn) << PAGE_SHIFT)

#define MM_PAGE_FLAG_LIST(X)                                                    \
	X(FREE, 0)                                                              \
	X(RESERVED, 1)                                                          \
	X(USED, 2)

#define MM_DEFINE_PAGE_FLAG(name, bit) PAGE_##name = (1U << (bit)),
enum {
	MM_PAGE_FLAG_LIST(MM_DEFINE_PAGE_FLAG)
};
#undef MM_DEFINE_PAGE_FLAG

typedef struct page page_t;

struct page {
	uint32_t flags;
	uint8_t order;
	page_t *next;
	page_t *prev;
};

static inline bool page_has_flags(const page_t *page, uint32_t flags)
{
	return (page->flags & flags) != 0U;
}

static inline void page_set_flags(page_t *page, uint32_t flags)
{
	page->flags |= flags;
}

static inline void page_clear_flags(page_t *page, uint32_t flags)
{
	page->flags &= ~flags;
}

static inline void page_set_state(page_t *page, uint32_t flags, uint8_t order)
{
	page->flags = flags;
	page->order = order;
	page->next = NULL;
	page->prev = NULL;
}

static inline void page_reset_links(page_t *page)
{
	page->next = NULL;
	page->prev = NULL;
}

#define MM_DEFINE_PAGE_FLAG_ACCESSORS(name, bit)                                \
	static inline bool Page##name(const page_t *page)                       \
	{                                                                      \
		return page_has_flags(page, PAGE_##name);                      \
	}                                                                      \
	static inline void SetPage##name(page_t *page)                         \
	{                                                                      \
		page_set_flags(page, PAGE_##name);                            \
	}                                                                      \
	static inline void ClearPage##name(page_t *page)                       \
	{                                                                      \
		page_clear_flags(page, PAGE_##name);                          \
	}

MM_PAGE_FLAG_LIST(MM_DEFINE_PAGE_FLAG_ACCESSORS)

#undef MM_DEFINE_PAGE_FLAG_ACCESSORS

static inline bool PageFree(const page_t *page)
{
	return page_has_flags(page, PAGE_FREE);
}

static inline bool PageReserved(const page_t *page)
{
	return page_has_flags(page, PAGE_RESERVED);
}

static inline bool PageUsed(const page_t *page)
{
	return page_has_flags(page, PAGE_USED);
}

static inline void SetPageFlags(page_t *page, uint32_t flags)
{
	page_set_flags(page, flags);
}

static inline void ClearPageFlags(page_t *page, uint32_t flags)
{
	page_clear_flags(page, flags);
}

static inline void SetPageFree(page_t *page)
{
	page_set_flags(page, PAGE_FREE);
}

static inline void ClearPageFree(page_t *page)
{
	page_clear_flags(page, PAGE_FREE);
}

static inline void SetPageReserved(page_t *page)
{
	page_set_flags(page, PAGE_RESERVED);
}

static inline void ClearPageReserved(page_t *page)
{
	page_clear_flags(page, PAGE_RESERVED);
}

static inline void SetPageUsed(page_t *page)
{
	page_set_flags(page, PAGE_USED);
}

static inline void ClearPageUsed(page_t *page)
{
	page_clear_flags(page, PAGE_USED);
}

#endif // MM_PAGE_H
