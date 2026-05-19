#ifndef DEV_FDT_H
#define DEV_FDT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct fdt {
	const void *blob;
	const uint8_t *struct_block;
	const uint8_t *struct_block_end;
	const char *strings_block;
	size_t strings_size;
};

struct fdt_node {
	uint32_t struct_offset;
	uint32_t parent_address_cells;
	uint32_t parent_size_cells;
};

struct fdt_prop {
	const void *data;
	uint32_t len;
};

bool fdt_init(struct fdt *fdt, const void *blob);
bool fdt_find_node_by_path(const struct fdt *fdt, const char *path,
						   struct fdt_node *out_node);
bool fdt_find_compatible(const struct fdt *fdt, const char *compatible,
						 struct fdt_node *out_node);
bool fdt_node_has_compatible(const struct fdt *fdt, const struct fdt_node *node,
							 const char *compatible);
bool fdt_get_property(const struct fdt *fdt, const struct fdt_node *node,
					  const char *name, struct fdt_prop *out_prop);
bool fdt_get_reg(const struct fdt *fdt, const struct fdt_node *node, unsigned index,
				 uintptr_t *out_addr, uint64_t *out_size);
bool fdt_get_stdout_path(const struct fdt *fdt, char *buffer, size_t buffer_size);
bool fdt_resolve_alias(const struct fdt *fdt, const char *alias, char *buffer,
					   size_t buffer_size);
bool fdt_get_u32(const struct fdt *fdt, const struct fdt_node *node,
				 const char *name, uint32_t *value);

#endif // DEV_FDT_H
