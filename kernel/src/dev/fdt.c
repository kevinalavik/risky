#include <dev/fdt.h>
#include <lib/string.h>

#define FDT_MAGIC 0xd00dfeedU

#define FDT_BEGIN_NODE 0x1U
#define FDT_END_NODE 0x2U
#define FDT_PROP 0x3U
#define FDT_NOP 0x4U
#define FDT_END 0x9U

#define FDT_DEFAULT_ADDRESS_CELLS 2U
#define FDT_DEFAULT_SIZE_CELLS 1U
#define FDT_MAX_DEPTH 32U
#define FDT_MAX_PATH 256U

struct fdt_header {
	uint32_t magic;
	uint32_t totalsize;
	uint32_t off_dt_struct;
	uint32_t off_dt_strings;
	uint32_t off_mem_rsvmap;
	uint32_t version;
	uint32_t last_comp_version;
	uint32_t boot_cpuid_phys;
	uint32_t size_dt_strings;
	uint32_t size_dt_struct;
};

struct fdt_walk_frame {
	size_t path_len;
	uint32_t child_address_cells;
	uint32_t child_size_cells;
};

struct fdt_walk_state {
	const struct fdt *fdt;
	const uint8_t *cursor;
	char path[FDT_MAX_PATH];
	struct fdt_walk_frame frames[FDT_MAX_DEPTH];
	uint32_t depth;
	size_t path_len;
};

static uint32_t fdt_read_be32(const void *ptr)
{
	const uint8_t *bytes = ptr;

	return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
		   ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3];
}

static bool fdt_copy_string(char *buffer, size_t buffer_size, const char *s,
							size_t len)
{
	if (buffer_size == 0 || len + 1 > buffer_size) {
		return false;
	}

	memcpy(buffer, s, len);
	buffer[len] = '\0';
	return true;
}

static const uint8_t *fdt_align4(const uint8_t *ptr)
{
	uintptr_t value = (uintptr_t)ptr;
	value = (value + 3U) & ~(uintptr_t)3U;
	return (const uint8_t *)value;
}

static bool fdt_bounded_strlen(const char *s, const uint8_t *limit, size_t *len)
{
	*len = strnlen(s, (size_t)(limit - (const uint8_t *)s));
	return (const uint8_t *)(s + *len) < limit;
}

static bool fdt_range_valid(size_t total_size, uint32_t offset, uint32_t size)
{
	return offset <= total_size && size <= total_size - offset;
}

static bool fdt_prop_name_valid(const struct fdt *fdt, uint32_t nameoff)
{
	size_t idx;

	if (nameoff >= fdt->strings_size) {
		return false;
	}

	for (idx = nameoff; idx < fdt->strings_size; idx++) {
		if (fdt->strings_block[idx] == '\0') {
			return true;
		}
	}

	return false;
}

static bool fdt_prop_is_string(const struct fdt_prop *prop)
{
	const char *s = prop->data;

	return prop->len > 0 && s[prop->len - 1] == '\0';
}

static bool fdt_prop_get_u32(const struct fdt_prop *prop, uint32_t *value)
{
	if (prop->len != sizeof(uint32_t)) {
		return false;
	}

	*value = fdt_read_be32(prop->data);
	return true;
}

static bool fdt_walk_init(const struct fdt *fdt, struct fdt_walk_state *state)
{
	memset(state, 0, sizeof(*state));
	state->fdt = fdt;
	state->cursor = fdt->struct_block;
	return true;
}

static bool fdt_walk_next_node(struct fdt_walk_state *state,
							   uint32_t *out_struct_offset,
							   uint32_t *out_parent_address_cells,
							   uint32_t *out_parent_size_cells)
{
	while (state->cursor < state->fdt->struct_block_end) {
		const uint8_t *token_ptr = state->cursor;
		uint32_t token;

		if ((size_t)(state->fdt->struct_block_end - state->cursor) <
			sizeof(uint32_t)) {
			return false;
		}

		token = fdt_read_be32(state->cursor);
		state->cursor += sizeof(uint32_t);

		switch (token) {
		case FDT_BEGIN_NODE: {
			const char *name = (const char *)state->cursor;
			size_t name_len;
			size_t new_len;

			if (state->depth >= FDT_MAX_DEPTH ||
				!fdt_bounded_strlen(name, state->fdt->struct_block_end, &name_len)) {
				return false;
			}

			*out_parent_address_cells =
				state->depth > 0
					? state->frames[state->depth - 1].child_address_cells
					: FDT_DEFAULT_ADDRESS_CELLS;
			*out_parent_size_cells =
				state->depth > 0
					? state->frames[state->depth - 1].child_size_cells
					: FDT_DEFAULT_SIZE_CELLS;

			state->frames[state->depth].path_len = state->path_len;
			state->frames[state->depth].child_address_cells =
				FDT_DEFAULT_ADDRESS_CELLS;
			state->frames[state->depth].child_size_cells =
				FDT_DEFAULT_SIZE_CELLS;

			if (state->depth == 0) {
				state->path[0] = '/';
				state->path[1] = '\0';
				state->path_len = 1;
			} else {
				new_len = state->path_len;
				if (new_len == 0 || state->path[new_len - 1] != '/') {
					if (new_len + 1 >= FDT_MAX_PATH) {
						return false;
					}

					state->path[new_len++] = '/';
				}

				if (new_len + name_len >= FDT_MAX_PATH) {
					return false;
				}

				memcpy(&state->path[new_len], name, name_len);
				new_len += name_len;
				state->path[new_len] = '\0';
				state->path_len = new_len;
			}

			state->depth++;
			state->cursor = fdt_align4(state->cursor + name_len + 1);
			if (state->cursor > state->fdt->struct_block_end) {
				return false;
			}

			*out_struct_offset =
				(uint32_t)(token_ptr - state->fdt->struct_block);
			return true;
		}
		case FDT_END_NODE:
			if (state->depth == 0) {
				return false;
			}

			state->depth--;
			state->path_len = state->frames[state->depth].path_len;
			state->path[state->path_len] = '\0';
			break;
		case FDT_PROP: {
			uint32_t len;
			uint32_t nameoff;
			const char *prop_name;
			struct fdt_prop prop;

			if ((size_t)(state->fdt->struct_block_end - state->cursor) <
				2 * sizeof(uint32_t)) {
				return false;
			}

			len = fdt_read_be32(state->cursor);
			nameoff = fdt_read_be32(state->cursor + sizeof(uint32_t));
			state->cursor += 2 * sizeof(uint32_t);
			if ((size_t)(state->fdt->struct_block_end - state->cursor) < len ||
				!fdt_prop_name_valid(state->fdt, nameoff) || state->depth == 0) {
				return false;
			}

			prop.data = state->cursor;
			prop.len = len;
			prop_name = state->fdt->strings_block + nameoff;
			state->cursor = fdt_align4(state->cursor + len);

			if (strcmp(prop_name, "#address-cells") == 0) {
				if (!fdt_prop_get_u32(&prop,
									  &state->frames[state->depth - 1]
										   .child_address_cells)) {
					return false;
				}
			} else if (strcmp(prop_name, "#size-cells") == 0) {
				if (!fdt_prop_get_u32(&prop,
									  &state->frames[state->depth - 1]
										   .child_size_cells)) {
					return false;
				}
			}
			break;
		}
		case FDT_NOP:
			break;
		case FDT_END:
			return false;
		default:
			return false;
		}
	}

	return false;
}

static bool fdt_node_payload(const struct fdt *fdt, const struct fdt_node *node,
							 const uint8_t **payload)
{
	const uint8_t *cursor = fdt->struct_block + node->struct_offset;
	const char *name;
	size_t name_len;

	if (cursor + sizeof(uint32_t) > fdt->struct_block_end ||
		fdt_read_be32(cursor) != FDT_BEGIN_NODE) {
		return false;
	}

	cursor += sizeof(uint32_t);
	name = (const char *)cursor;
	if (!fdt_bounded_strlen(name, fdt->struct_block_end, &name_len)) {
		return false;
	}

	cursor = fdt_align4(cursor + name_len + 1);
	if (cursor > fdt->struct_block_end) {
		return false;
	}

	*payload = cursor;
	return true;
}

bool fdt_init(struct fdt *fdt, const void *blob)
{
	const struct fdt_header *header = blob;
	size_t total_size;
	uint32_t off_dt_struct;
	uint32_t off_dt_strings;
	uint32_t size_dt_struct;
	uint32_t size_dt_strings;

	if (fdt == NULL || blob == NULL) {
		return false;
	}

	if (fdt_read_be32(&header->magic) != FDT_MAGIC) {
		return false;
	}

	total_size = fdt_read_be32(&header->totalsize);
	off_dt_struct = fdt_read_be32(&header->off_dt_struct);
	off_dt_strings = fdt_read_be32(&header->off_dt_strings);
	size_dt_struct = fdt_read_be32(&header->size_dt_struct);
	size_dt_strings = fdt_read_be32(&header->size_dt_strings);

	if (!fdt_range_valid(total_size, off_dt_struct, size_dt_struct) ||
		!fdt_range_valid(total_size, off_dt_strings, size_dt_strings)) {
		return false;
	}

	fdt->blob = blob;
	fdt->struct_block = (const uint8_t *)blob + off_dt_struct;
	fdt->struct_block_end = fdt->struct_block + size_dt_struct;
	fdt->strings_block = (const char *)blob + off_dt_strings;
	fdt->strings_size = size_dt_strings;
	return true;
}

bool fdt_get_property(const struct fdt *fdt, const struct fdt_node *node,
					  const char *name, struct fdt_prop *out_prop)
{
	const uint8_t *cursor;
	uint32_t depth = 0;

	if (fdt == NULL || node == NULL || name == NULL || out_prop == NULL ||
		!fdt_node_payload(fdt, node, &cursor)) {
		return false;
	}

	while (cursor < fdt->struct_block_end) {
		uint32_t token = fdt_read_be32(cursor);
		cursor += sizeof(uint32_t);

		switch (token) {
		case FDT_BEGIN_NODE: {
			const char *child_name = (const char *)cursor;
			size_t child_name_len;

			if (!fdt_bounded_strlen(child_name, fdt->struct_block_end,
									&child_name_len)) {
				return false;
			}

			cursor = fdt_align4(cursor + child_name_len + 1);
			depth++;
			break;
		}
		case FDT_END_NODE:
			if (depth == 0) {
				return false;
			}

			depth--;
			break;
		case FDT_PROP: {
			uint32_t len;
			uint32_t nameoff;
			const char *prop_name;
			const void *data;

			if ((size_t)(fdt->struct_block_end - cursor) < 2 * sizeof(uint32_t)) {
				return false;
			}

			len = fdt_read_be32(cursor);
			nameoff = fdt_read_be32(cursor + sizeof(uint32_t));
			cursor += 2 * sizeof(uint32_t);
			if ((size_t)(fdt->struct_block_end - cursor) < len ||
				!fdt_prop_name_valid(fdt, nameoff)) {
				return false;
			}

			data = cursor;
			prop_name = fdt->strings_block + nameoff;
			cursor = fdt_align4(cursor + len);

			if (depth == 0 && strcmp(prop_name, name) == 0) {
				out_prop->data = data;
				out_prop->len = len;
				return true;
			}
			break;
		}
		case FDT_NOP:
			break;
		case FDT_END:
			return false;
		default:
			return false;
		}
	}

	return false;
}

bool fdt_node_has_compatible(const struct fdt *fdt, const struct fdt_node *node,
							 const char *compatible)
{
	struct fdt_prop prop;
	size_t offset = 0;

	if (!fdt_get_property(fdt, node, "compatible", &prop)) {
		return false;
	}

	while (offset < prop.len) {
		const char *entry = (const char *)prop.data + offset;
		size_t len = strlen(entry);

		if (offset + len >= prop.len) {
			return false;
		}

		if (strcmp(entry, compatible) == 0) {
			return true;
		}

		offset += len + 1;
	}

	return false;
}

bool fdt_find_node_by_path(const struct fdt *fdt, const char *path,
						   struct fdt_node *out_node)
{
	struct fdt_walk_state state;
	uint32_t struct_offset;
	uint32_t parent_address_cells;
	uint32_t parent_size_cells;

	if (fdt == NULL || path == NULL || out_node == NULL) {
		return false;
	}

	fdt_walk_init(fdt, &state);
	while (fdt_walk_next_node(&state, &struct_offset, &parent_address_cells,
							  &parent_size_cells)) {
		if (strcmp(state.path, path) == 0) {
			out_node->struct_offset = struct_offset;
			out_node->parent_address_cells = parent_address_cells;
			out_node->parent_size_cells = parent_size_cells;
			return true;
		}
	}

	return false;
}

bool fdt_find_compatible(const struct fdt *fdt, const char *compatible,
						 struct fdt_node *out_node)
{
	struct fdt_walk_state state;
	uint32_t struct_offset;
	uint32_t parent_address_cells;
	uint32_t parent_size_cells;
	struct fdt_node node;

	if (fdt == NULL || compatible == NULL || out_node == NULL) {
		return false;
	}

	fdt_walk_init(fdt, &state);
	while (fdt_walk_next_node(&state, &struct_offset, &parent_address_cells,
							  &parent_size_cells)) {
		node.struct_offset = struct_offset;
		node.parent_address_cells = parent_address_cells;
		node.parent_size_cells = parent_size_cells;

		if (fdt_node_has_compatible(fdt, &node, compatible)) {
			*out_node = node;
			return true;
		}
	}

	return false;
}

bool fdt_get_reg(const struct fdt *fdt, const struct fdt_node *node, unsigned index,
				 uintptr_t *out_addr, uint64_t *out_size)
{
	struct fdt_prop prop;
	const uint8_t *cells;
	uint32_t entry_cells;
	uint32_t cell_count;
	uint32_t entry_index;
	uint64_t addr = 0;
	uint64_t size = 0;

	if (fdt == NULL || node == NULL || out_addr == NULL || out_size == NULL ||
		!fdt_get_property(fdt, node, "reg", &prop) ||
		node->parent_address_cells > 2 || node->parent_size_cells > 2) {
		return false;
	}

	entry_cells = node->parent_address_cells + node->parent_size_cells;
	if (entry_cells == 0 || prop.len % sizeof(uint32_t) != 0) {
		return false;
	}

	cell_count = prop.len / sizeof(uint32_t);
	if ((index + 1U) * entry_cells > cell_count) {
		return false;
	}

	cells = (const uint8_t *)prop.data + index * entry_cells * sizeof(uint32_t);
	for (entry_index = 0; entry_index < node->parent_address_cells;
		 entry_index++) {
		addr = (addr << 32) | fdt_read_be32(cells);
		cells += sizeof(uint32_t);
	}

	for (entry_index = 0; entry_index < node->parent_size_cells;
		 entry_index++) {
		size = (size << 32) | fdt_read_be32(cells);
		cells += sizeof(uint32_t);
	}

	*out_addr = (uintptr_t)addr;
	*out_size = size;
	return true;
}

bool fdt_get_stdout_path(const struct fdt *fdt, char *buffer, size_t buffer_size)
{
	struct fdt_node chosen;
	struct fdt_prop prop;
	const char *stdout_path;
	size_t len;

	if (fdt == NULL || buffer == NULL) {
		return false;
	}

	if (!fdt_find_node_by_path(fdt, "/chosen", &chosen) ||
		(!fdt_get_property(fdt, &chosen, "stdout-path", &prop) &&
		 !fdt_get_property(fdt, &chosen, "linux,stdout-path", &prop)) ||
		!fdt_prop_is_string(&prop)) {
		return false;
	}

	stdout_path = prop.data;
	if (strchr(stdout_path, ':') != NULL) {
		len = (size_t)(strchr(stdout_path, ':') - stdout_path);
	} else {
		len = strlen(stdout_path);
	}
	return fdt_copy_string(buffer, buffer_size, stdout_path, len);
}

bool fdt_resolve_alias(const struct fdt *fdt, const char *alias, char *buffer,
					   size_t buffer_size)
{
	struct fdt_node aliases;
	struct fdt_prop prop;
	size_t len;

	if (fdt == NULL || alias == NULL || buffer == NULL) {
		return false;
	}

	if (!fdt_find_node_by_path(fdt, "/aliases", &aliases) ||
		!fdt_get_property(fdt, &aliases, alias, &prop) ||
		!fdt_prop_is_string(&prop)) {
		return false;
	}

	len = strlen((const char *)prop.data);
	return fdt_copy_string(buffer, buffer_size, prop.data, len);
}

bool fdt_get_u32(const struct fdt *fdt, const struct fdt_node *node,
				 const char *name, uint32_t *value)
{
	struct fdt_prop prop;

	if (fdt == NULL || node == NULL || name == NULL || value == NULL ||
		!fdt_get_property(fdt, node, name, &prop) ||
		!fdt_prop_get_u32(&prop, value)) {
		return false;
	}
	return true;
}
