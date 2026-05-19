#include <arch/byteorder.h>
#include <dev/fw_cfg.h>
#include <dev/ramfb.h>

#define RAMFB_FW_CFG_FILE "etc/ramfb"
#define FOURCC(a, b, c, d)                                                     \
  ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) |              \
   ((uint32_t)(d) << 24))

#define DRM_FORMAT_XRGB8888 FOURCC('X', 'R', '2', '4')

struct ramfb_cfg {
  uint64_t address;
  uint32_t fourcc;
  uint32_t flags;
  uint32_t width;
  uint32_t height;
  uint32_t stride;
} __attribute__((packed));

int ramfb_configure(const struct framebuffer *framebuffer) {
  uint16_t selector = 0;
  uint32_t size = 0;

  if (fw_cfg_find_file(RAMFB_FW_CFG_FILE, &selector, &size) != 0) {
    return -1;
  }

  if (size < sizeof(struct ramfb_cfg)) {
    return -1;
  }

  struct ramfb_cfg config = {
      .address = cpu_to_be64((uint64_t)framebuffer->framebuffer),
      .fourcc = cpu_to_be32(DRM_FORMAT_XRGB8888),
      .flags = 0,
      .width = cpu_to_be32((uint32_t)framebuffer->width),
      .height = cpu_to_be32((uint32_t)framebuffer->height),
      .stride = cpu_to_be32((uint32_t)framebuffer->pitch),
  };

  return fw_cfg_dma_write(selector, &config, sizeof(config));
}
