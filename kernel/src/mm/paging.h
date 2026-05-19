#ifndef MM_PAGING_H
#define MM_PAGING_H

#include <stdbool.h>
#include <stdint.h>

#define PAGING_FLAG_READ (1UL << 0)
#define PAGING_FLAG_WRITE (1UL << 1)
#define PAGING_FLAG_EXEC (1UL << 2)
#define PAGING_FLAG_GLOBAL (1UL << 3)

bool paging_early_init(void);
bool paging_init(void);
bool paging_map_page(uintptr_t virt_addr, uintptr_t phys_addr, uint64_t flags);
bool paging_map_range(uintptr_t virt_addr, uintptr_t phys_addr, uint64_t size,
					  uint64_t flags);
bool paging_map_mmio(uintptr_t phys_addr, uint64_t size);
uintptr_t paging_root_phys(void);

#endif // MM_PAGING_H
