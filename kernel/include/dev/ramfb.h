#ifndef DEV_RAMFB_H
#define DEV_RAMFB_H

#include <sys/types.h>

struct framebuffer {
  uint32_t *framebuffer;
  size_t width;
  size_t height;
  size_t pitch;
  uint8_t red_mask_size;
  uint8_t red_mask_shift;
  uint8_t green_mask_size;
  uint8_t green_mask_shift;
  uint8_t blue_mask_size;
  uint8_t blue_mask_shift;
};

int ramfb_configure(const struct framebuffer *framebuffer);

#endif // DEV_RAMFB_H
