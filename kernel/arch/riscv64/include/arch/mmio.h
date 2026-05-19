#ifndef ARCH_MMIO_H
#define ARCH_MMIO_H

#include <arch/byteorder.h>
#include <sys/types.h>

static inline uint8_t mmio_read8(uint64_t address) {
  return *(volatile uint8_t *)address;
}

static inline void mmio_write8(uint64_t address, uint8_t value) {
  *(volatile uint8_t *)address = value;
}

static inline void mmio_write16_be(uint64_t address, uint16_t value) {
  *(volatile uint16_t *)address = cpu_to_be16(value);
}

static inline void mmio_write64_be(uint64_t address, uint64_t value) {
  *(volatile uint64_t *)address = cpu_to_be64(value);
}

#endif // ARCH_MMIO_H
