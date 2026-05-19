#ifndef DEV_DTB_H
#define DEV_DTB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct limine_dtb_response;

struct dtb_node {
	uint32_t struct_offset;
	uint32_t parent_address_cells;
	uint32_t parent_size_cells;
};

bool dtb_init(const struct limine_dtb_response *response);
bool dtb_find_node_by_path(const char *path, struct dtb_node *out_node);
bool dtb_find_compatible(const char *compatible, struct dtb_node *out_node);
bool dtb_node_has_compatible(const struct dtb_node *node,
							 const char *compatible);
bool dtb_get_reg(const struct dtb_node *node, unsigned index,
				 uintptr_t *out_addr, uint64_t *out_size);
bool dtb_get_stdout_path(char *buffer, size_t buffer_size);
bool dtb_resolve_alias(const char *alias, char *buffer, size_t buffer_size);
bool dtb_get_u32(const struct dtb_node *node, const char *name, uint32_t *value);

#endif // DEV_DTB_H
