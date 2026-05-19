#include <arch/byteorder.h>
#include <arch/mmio.h>
#include <dev/fw_cfg.h>

#define FW_CFG_BASE 0x10100000UL
#define FW_CFG_DATA (FW_CFG_BASE + 0x00)
#define FW_CFG_SELECTOR (FW_CFG_BASE + 0x08)
#define FW_CFG_DMA (FW_CFG_BASE + 0x10)

#define FW_CFG_FILE_DIR 0x0019
#define FW_CFG_DMA_CTL_ERROR 0x01
#define FW_CFG_DMA_CTL_SELECT 0x08
#define FW_CFG_DMA_CTL_WRITE 0x10

struct fw_cfg_file {
  uint32_t size;
  uint16_t selector;
  uint16_t reserved;
  char name[56];
} __attribute__((packed));

struct fw_cfg_dma_access {
  uint32_t control;
  uint32_t length;
  uint64_t address;
} __attribute__((packed, aligned(8)));

static void fw_cfg_select(uint16_t selector) {
  mmio_write16_be(FW_CFG_SELECTOR, selector);
}

static void fw_cfg_read_bytes(void *buffer, uint32_t length) {
  uint8_t *bytes = (uint8_t *)buffer;

  for (uint32_t i = 0; i < length; ++i) {
    bytes[i] = mmio_read8(FW_CFG_DATA);
  }
}

static int names_equal(const char *left, const char *right) {
  while (*left != '\0' && *right != '\0') {
    if (*left != *right) {
      return 0;
    }

    ++left;
    ++right;
  }

  return *left == *right;
}

int fw_cfg_find_file(const char *name, uint16_t *selector, uint32_t *size) {
  uint32_t count_be = 0;

  fw_cfg_select(FW_CFG_FILE_DIR);
  fw_cfg_read_bytes(&count_be, sizeof(count_be));

  uint32_t count = be32_to_cpu(count_be);
  for (uint32_t i = 0; i < count; ++i) {
    struct fw_cfg_file file;

    fw_cfg_read_bytes(&file, sizeof(file));
    if (!names_equal(file.name, name)) {
      continue;
    }

    *selector = be16_to_cpu(file.selector);
    *size = be32_to_cpu(file.size);
    return 0;
  }

  return -1;
}

int fw_cfg_dma_write(uint16_t selector, const void *data, uint32_t length) {
  static volatile struct fw_cfg_dma_access dma;

  uint32_t control = ((uint32_t)selector << 16) | FW_CFG_DMA_CTL_SELECT |
                     FW_CFG_DMA_CTL_WRITE;

  dma.control = cpu_to_be32(control);
  dma.length = cpu_to_be32(length);
  dma.address = cpu_to_be64((uint64_t)data);

  __asm__ volatile("fence rw, rw" ::: "memory");
  mmio_write64_be(FW_CFG_DMA, (uint64_t)&dma);

  for (;;) {
    uint32_t status = be32_to_cpu(dma.control);

    if (status == 0) {
      return 0;
    }

    if ((status & FW_CFG_DMA_CTL_ERROR) != 0) {
      return -1;
    }
  }
}
