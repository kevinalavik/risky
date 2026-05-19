#include <dev/dtb.h>
#include <dev/fdt.h>
#include <limine.h>
#include <lib/printk.h>
#include <lib/string.h>

static struct fdt dtb_fdt;
static bool dtb_ready;

static void dtb_node_to_fdt(const struct dtb_node *src, struct fdt_node *dst)
{
	dst->struct_offset = src->struct_offset;
	dst->parent_address_cells = src->parent_address_cells;
	dst->parent_size_cells = src->parent_size_cells;
}

static void fdt_node_to_dtb(const struct fdt_node *src, struct dtb_node *dst)
{
	dst->struct_offset = src->struct_offset;
	dst->parent_address_cells = src->parent_address_cells;
	dst->parent_size_cells = src->parent_size_cells;
}

bool dtb_init(const struct limine_dtb_response *response)
{
	dtb_ready = false;
	memset(&dtb_fdt, 0, sizeof(dtb_fdt));

	if (response == NULL) {
		klog("dtb: missing Limine dtb response\n");
		return false;
	}

	if (response->dtb_ptr == NULL) {
		klog("dtb: Limine dtb_ptr is NULL\n");
		return false;
	}

	dtb_ready = fdt_init(&dtb_fdt, response->dtb_ptr);
	if (!dtb_ready) {
		klog("dtb: invalid fdt blob at %p\n", response->dtb_ptr);
	}

	return dtb_ready;
}

bool dtb_find_node_by_path(const char *path, struct dtb_node *out_node)
{
	struct fdt_node node;

	if (path == NULL || out_node == NULL || !dtb_ready ||
		!fdt_find_node_by_path(&dtb_fdt, path, &node)) {
		return false;
	}

	fdt_node_to_dtb(&node, out_node);
	return true;
}

bool dtb_find_compatible(const char *compatible, struct dtb_node *out_node)
{
	struct fdt_node node;

	if (compatible == NULL || out_node == NULL || !dtb_ready ||
		!fdt_find_compatible(&dtb_fdt, compatible, &node)) {
		return false;
	}

	fdt_node_to_dtb(&node, out_node);
	return true;
}

bool dtb_node_has_compatible(const struct dtb_node *node,
							 const char *compatible)
{
	struct fdt_node fdt_node;

	if (node == NULL || compatible == NULL || !dtb_ready) {
		return false;
	}

	dtb_node_to_fdt(node, &fdt_node);
	return fdt_node_has_compatible(&dtb_fdt, &fdt_node, compatible);
}

bool dtb_get_reg(const struct dtb_node *node, unsigned index,
				 uintptr_t *out_addr, uint64_t *out_size)
{
	struct fdt_node fdt_node;

	if (node == NULL || out_addr == NULL || out_size == NULL || !dtb_ready) {
		return false;
	}

	dtb_node_to_fdt(node, &fdt_node);
	return fdt_get_reg(&dtb_fdt, &fdt_node, index, out_addr, out_size);
}

bool dtb_get_stdout_path(char *buffer, size_t buffer_size)
{
	if (buffer == NULL || !dtb_ready) {
		return false;
	}

	return fdt_get_stdout_path(&dtb_fdt, buffer, buffer_size);
}

bool dtb_resolve_alias(const char *alias, char *buffer, size_t buffer_size)
{
	if (alias == NULL || buffer == NULL || !dtb_ready) {
		return false;
	}

	return fdt_resolve_alias(&dtb_fdt, alias, buffer, buffer_size);
}

bool dtb_get_u32(const struct dtb_node *node, const char *name, uint32_t *value)
{
	struct fdt_node fdt_node;

	if (node == NULL || name == NULL || value == NULL || !dtb_ready) {
		return false;
	}

	dtb_node_to_fdt(node, &fdt_node);
	return fdt_get_u32(&dtb_fdt, &fdt_node, name, value);
}
