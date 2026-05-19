#include <dev/ramfb.h>
#include <fb.h>
#include <flanterm.h>
#include <machine/uart.h>
#include <sys/types.h>

#define FRAMEBUFFER_WIDTH 640UL
#define FRAMEBUFFER_HEIGHT 480UL
#define FRAMEBUFFER_PITCH (FRAMEBUFFER_WIDTH * sizeof(uint32_t))

static uint32_t framebuffer_storage[FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT]
    __attribute__((aligned(4096)));

static struct framebuffer boot_framebuffer = {
    .framebuffer = framebuffer_storage,
    .width = FRAMEBUFFER_WIDTH,
    .height = FRAMEBUFFER_HEIGHT,
    .pitch = FRAMEBUFFER_PITCH,
    .red_mask_size = 8,
    .red_mask_shift = 16,
    .green_mask_size = 8,
    .green_mask_shift = 8,
    .blue_mask_size = 8,
    .blue_mask_shift = 0,
};

static void uart_puts(const char *str) {
  while (*str != '\0') {
    if (*str == '\n') {
      uart_putchar('\r');
    }

    uart_putchar(*str++);
  }
}

void kmain(uint64_t hartid, uint64_t dtb_pa) {
  (void)hartid;
  (void)dtb_pa;

  uart_init();

  uart_puts("\n");
  uart_puts("welcome to risky\n");

  if (ramfb_configure(&boot_framebuffer) == 0) {
    struct flanterm_context *terminal = flanterm_fb_init(
        0, 0, boot_framebuffer.framebuffer, boot_framebuffer.width,
        boot_framebuffer.height, boot_framebuffer.pitch,
        boot_framebuffer.red_mask_size, boot_framebuffer.red_mask_shift,
        boot_framebuffer.green_mask_size, boot_framebuffer.green_mask_shift,
        boot_framebuffer.blue_mask_size, boot_framebuffer.blue_mask_shift, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, FLANTERM_FB_ROTATE_0);

    if (terminal != 0) {
      static const char hello[] = "Hello via ramfb!\r\n";
      flanterm_write(terminal, hello, sizeof(hello) - 1);
    } else {
      uart_puts("framebuffer terminal unavailable\n");
    }
  } else {
    uart_puts("framebuffer unavailable\n");
  }

  for (;;) {
    __asm__ volatile("wfi");
  }
}
