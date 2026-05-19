#ifndef DT_FDT_H
#define DT_FDT_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

struct fdt {
	const uint8_t *blob;
	const uint8_t *struct_block;
	const uint8_t *struct_end;
	const char *strings_block;
	uint32_t strings_size;
};

struct fdt_node {
	const struct fdt *fdt;
	const uint8_t *begin;
	const uint8_t *properties;
	const uint8_t *end;
	const char *name;
};

struct fdt_property {
	const char *name;
	const void *data;
	uint32_t length;
};

/* Global FDT — call fdt_global_init() once in kmain, then use fdt_get() anywhere. */
bool fdt_global_init(const void *blob);
const struct fdt *fdt_get(void);

bool fdt_init(struct fdt *fdt, const void *blob);
bool fdt_root(const struct fdt *fdt, struct fdt_node *node);
bool fdt_find_child(const struct fdt_node *parent, const char *name,
					struct fdt_node *child);
bool fdt_find_path(const struct fdt *fdt, const char *path,
				   struct fdt_node *node);
bool fdt_get_property(const struct fdt_node *node, const char *name,
					  struct fdt_property *property);
bool fdt_property_read_u32(const struct fdt_property *property,
						   uint32_t *value);
bool fdt_property_read_u64(const struct fdt_property *property,
						   uint64_t *value);
bool fdt_read_u32(const struct fdt_node *node, const char *property_name,
				  uint32_t *value);
bool fdt_read_u64(const struct fdt_node *node, const char *property_name,
				  uint64_t *value);

#endif // DT_FDT_H
