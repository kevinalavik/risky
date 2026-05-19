#include <arch/byteorder.h>
#include <dt/fdt.h>
#include <stdint.h>

#define FDT_MAGIC 0xd00dfeedU

#define FDT_BEGIN_NODE 1U
#define FDT_END_NODE 2U
#define FDT_PROP 3U
#define FDT_NOP 4U
#define FDT_END 9U

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

struct fdt_prop_header {
  uint32_t len;
  uint32_t nameoff;
};

static uint32_t read_be32(const void *address) {
  return be32_to_cpu(*(const uint32_t *)address);
}

static const uint8_t *align4(const uint8_t *ptr) {
  uintptr_t value = (uintptr_t)ptr;
  value = (value + 3UL) & ~3UL;
  return (const uint8_t *)value;
}

static bool streq(const char *lhs, const char *rhs) {
  while (*lhs != '\0' && *rhs != '\0') {
    if (*lhs != *rhs) {
      return false;
    }

    lhs++;
    rhs++;
  }

  return *lhs == *rhs;
}

static bool component_matches(const char *name, size_t component_length,
                              const char *component) {
  size_t index = 0;

  while (index < component_length && component[index] != '\0') {
    if (name[index] != component[index]) {
      return false;
    }

    index++;
  }

  if (index != component_length) {
    return false;
  }

  return name[index] == '\0';
}

static bool fdt_valid_range(const struct fdt *fdt, const uint8_t *ptr,
                            size_t size) {
  return ptr >= fdt->struct_block && size <= (size_t)(fdt->struct_end - ptr);
}

static bool fdt_valid_string(const struct fdt *fdt, uint32_t nameoff,
                             const char **name) {
  const char *str;
  uint32_t index;

  if (nameoff >= fdt->strings_size) {
    return false;
  }

  str = fdt->strings_block + nameoff;

  for (index = nameoff; index < fdt->strings_size; index++) {
    if (fdt->strings_block[index] == '\0') {
      *name = str;
      return true;
    }
  }

  return false;
}

static bool fdt_parse_node_at(const struct fdt *fdt, const uint8_t *ptr,
                              struct fdt_node *node) {
  const uint8_t *scan = ptr;
  const char *name;
  unsigned int depth = 0;

  if (!fdt_valid_range(fdt, scan, sizeof(uint32_t)) ||
      read_be32(scan) != FDT_BEGIN_NODE) {
    return false;
  }

  scan += sizeof(uint32_t);
  name = (const char *)scan;

  while (scan < fdt->struct_end && *scan != '\0') {
    scan++;
  }

  if (scan >= fdt->struct_end) {
    return false;
  }

  scan = align4(scan + 1);

  if (node != NULL) {
    node->fdt = fdt;
    node->begin = ptr;
    node->properties = scan;
    node->name = name;
  }

  scan = ptr;

  while (scan < fdt->struct_end) {
    uint32_t token = read_be32(scan);
    scan += sizeof(uint32_t);

    switch (token) {
    case FDT_BEGIN_NODE:
      depth++;
      while (scan < fdt->struct_end && *scan != '\0') {
        scan++;
      }

      if (scan >= fdt->struct_end) {
        return false;
      }

      scan = align4(scan + 1);
      break;
    case FDT_END_NODE:
      if (depth == 0) {
        return false;
      }

      depth--;
      if (depth == 0) {
        if (node != NULL) {
          node->end = scan;
        }
        return true;
      }
      break;
    case FDT_PROP: {
      const struct fdt_prop_header *prop_header =
          (const struct fdt_prop_header *)scan;
      uint32_t length;

      if (!fdt_valid_range(fdt, scan, sizeof(struct fdt_prop_header))) {
        return false;
      }

      length = read_be32(&prop_header->len);
      scan += sizeof(struct fdt_prop_header);

      if (!fdt_valid_range(fdt, scan, length)) {
        return false;
      }

      scan = align4(scan + length);
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

bool fdt_init(struct fdt *fdt, const void *blob) {
  const struct fdt_header *header;
  const uint8_t *base = (const uint8_t *)blob;
  uint32_t totalsize;
  uint32_t off_dt_struct;
  uint32_t off_dt_strings;
  uint32_t size_dt_struct;
  uint32_t size_dt_strings;

  if (fdt == NULL || blob == NULL) {
    return false;
  }

  header = (const struct fdt_header *)blob;
  if (read_be32(&header->magic) != FDT_MAGIC) {
    return false;
  }

  totalsize = read_be32(&header->totalsize);
  off_dt_struct = read_be32(&header->off_dt_struct);
  off_dt_strings = read_be32(&header->off_dt_strings);
  size_dt_struct = read_be32(&header->size_dt_struct);
  size_dt_strings = read_be32(&header->size_dt_strings);

  if (off_dt_struct > totalsize || size_dt_struct > totalsize - off_dt_struct) {
    return false;
  }

  if (off_dt_strings > totalsize ||
      size_dt_strings > totalsize - off_dt_strings) {
    return false;
  }

  fdt->blob = base;
  fdt->struct_block = base + off_dt_struct;
  fdt->struct_end = fdt->struct_block + size_dt_struct;
  fdt->strings_block = (const char *)(base + off_dt_strings);
  fdt->strings_size = size_dt_strings;

  return true;
}

bool fdt_root(const struct fdt *fdt, struct fdt_node *node) {
  if (fdt == NULL || node == NULL) {
    return false;
  }

  return fdt_parse_node_at(fdt, fdt->struct_block, node);
}

bool fdt_find_child(const struct fdt_node *parent, const char *name,
                    struct fdt_node *child) {
  const struct fdt *fdt;
  const uint8_t *ptr;

  if (parent == NULL || name == NULL || child == NULL) {
    return false;
  }

  fdt = parent->fdt;
  ptr = parent->properties;

  while (ptr < parent->end) {
    uint32_t token;

    if (!fdt_valid_range(fdt, ptr, sizeof(uint32_t))) {
      return false;
    }

    token = read_be32(ptr);

    if (token == FDT_PROP) {
      const struct fdt_prop_header *prop_header =
          (const struct fdt_prop_header *)(ptr + sizeof(uint32_t));
      uint32_t length;

      if (!fdt_valid_range(fdt, ptr + sizeof(uint32_t),
                           sizeof(struct fdt_prop_header))) {
        return false;
      }

      length = read_be32(&prop_header->len);
      ptr += sizeof(uint32_t) + sizeof(struct fdt_prop_header);

      if (!fdt_valid_range(fdt, ptr, length)) {
        return false;
      }

      ptr = align4(ptr + length);
      continue;
    }

    if (token == FDT_NOP) {
      ptr += sizeof(uint32_t);
      continue;
    }

    if (token == FDT_BEGIN_NODE) {
      struct fdt_node candidate;

      if (!fdt_parse_node_at(fdt, ptr, &candidate)) {
        return false;
      }

      if (streq(candidate.name, name)) {
        *child = candidate;
        return true;
      }

      ptr = candidate.end;
      continue;
    }

    if (token == FDT_END_NODE) {
      return false;
    }

    return false;
  }

  return false;
}

bool fdt_find_path(const struct fdt *fdt, const char *path,
                   struct fdt_node *node) {
  struct fdt_node current;
  const char *component;

  if (fdt == NULL || path == NULL || node == NULL || path[0] != '/') {
    return false;
  }

  if (!fdt_root(fdt, &current)) {
    return false;
  }

  if (path[1] == '\0') {
    *node = current;
    return true;
  }

  component = path + 1;
  while (*component != '\0') {
    const char *next = component;
    size_t length = 0;
    struct fdt_node child;
    const uint8_t *ptr = current.properties;
    bool found = false;

    while (*next != '\0' && *next != '/') {
      next++;
      length++;
    }

    while (ptr < current.end) {
      uint32_t token = read_be32(ptr);

      if (token == FDT_PROP) {
        const struct fdt_prop_header *prop_header =
            (const struct fdt_prop_header *)(ptr + sizeof(uint32_t));
        uint32_t prop_length = read_be32(&prop_header->len);
        ptr += sizeof(uint32_t) + sizeof(struct fdt_prop_header);
        ptr = align4(ptr + prop_length);
        continue;
      }

      if (token == FDT_NOP) {
        ptr += sizeof(uint32_t);
        continue;
      }

      if (token == FDT_BEGIN_NODE) {
        if (!fdt_parse_node_at(fdt, ptr, &child)) {
          return false;
        }

        if (component_matches(child.name, length, component)) {
          current = child;
          found = true;
          break;
        }

        ptr = child.end;
        continue;
      }

      return false;
    }

    if (!found) {
      return false;
    }

    if (*next == '/') {
      component = next + 1;
      continue;
    }

    *node = current;
    return true;
  }

  return false;
}

bool fdt_get_property(const struct fdt_node *node, const char *name,
                      struct fdt_property *property) {
  const struct fdt *fdt;
  const uint8_t *ptr;

  if (node == NULL || name == NULL || property == NULL) {
    return false;
  }

  fdt = node->fdt;
  ptr = node->properties;

  while (ptr < node->end) {
    uint32_t token;

    if (!fdt_valid_range(fdt, ptr, sizeof(uint32_t))) {
      return false;
    }

    token = read_be32(ptr);
    ptr += sizeof(uint32_t);

    if (token == FDT_NOP) {
      continue;
    }

    if (token == FDT_PROP) {
      const struct fdt_prop_header *prop_header =
          (const struct fdt_prop_header *)ptr;
      uint32_t length;
      uint32_t nameoff;
      const char *property_name;

      if (!fdt_valid_range(fdt, ptr, sizeof(struct fdt_prop_header))) {
        return false;
      }

      length = read_be32(&prop_header->len);
      nameoff = read_be32(&prop_header->nameoff);
      ptr += sizeof(struct fdt_prop_header);

      if (!fdt_valid_range(fdt, ptr, length) ||
          !fdt_valid_string(fdt, nameoff, &property_name)) {
        return false;
      }

      if (streq(property_name, name)) {
        property->name = property_name;
        property->data = ptr;
        property->length = length;
        return true;
      }

      ptr = align4(ptr + length);
      continue;
    }

    if (token == FDT_BEGIN_NODE || token == FDT_END_NODE) {
      return false;
    }

    return false;
  }

  return false;
}

bool fdt_property_read_u32(const struct fdt_property *property, uint32_t *value) {
  if (property == NULL || value == NULL || property->length < sizeof(uint32_t)) {
    return false;
  }

  *value = read_be32(property->data);
  return true;
}

bool fdt_read_u32(const struct fdt_node *node, const char *property_name,
                  uint32_t *value) {
  struct fdt_property property;

  if (!fdt_get_property(node, property_name, &property)) {
    return false;
  }

  return fdt_property_read_u32(&property, value);
}
