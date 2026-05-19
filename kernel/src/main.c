#include <stddef.h>
#include <stdint.h>
#include <limine.h>
#include <flanterm.h>
#include <flanterm_backends/fb.h>
#include <cpu.h>
#include <dev/dtb.h>
#include <dev/uart.h>
#include <boot/req.h>
#include <lib/printk.h>
#include <lib/time.h>
#include <mm/paging.h>
#include <mm/pfndb.h>
#include <mm/pmm.h>
#include <sbi.h>

static struct flanterm_context *g_ft_ctx;

void printk_putc(char c)
{
	if (uart_is_ready())
		uart_putc(c);
	else
		sbi_legacy_console_putchar((int)c);

	if (g_ft_ctx != NULL)
		flanterm_write(g_ft_ctx, &c, 1);
}

void kmain(void)
{
	if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision)) {
		hlt();
	}

	ktime_init();
	pfndb_init(memmap_request.response);
	if (!pmm_init()) {
		klog("boot: pmm_init failed");
		hlt();
	}

	if (!dtb_init(dtb_request.response))
		klog("boot: dtb_init failed");

	if (!paging_early_init()) {
		klog("boot: paging_early_init failed");
		hlt();
	}

	if (!uart_init()) {
		klog("boot: uart_init failed");
		hlt();
	}

	if (!paging_init()) {
		klog("boot: paging_init failed");
		hlt();
	}

	if (framebuffer_request.response == NULL ||
		framebuffer_request.response->framebuffer_count < 1) {
		klog("early: failed to get framebuffer, no ramfb?");
	} else {
		struct limine_framebuffer *fb =
			framebuffer_request.response->framebuffers[0];

		g_ft_ctx = flanterm_fb_init(
			NULL, NULL, fb->address, fb->width, fb->height, fb->pitch,
			fb->red_mask_size, fb->red_mask_shift, fb->green_mask_size,
			fb->green_mask_shift, fb->blue_mask_size, fb->blue_mask_shift, NULL,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 1, 0, 0, 0, 0);

		if (g_ft_ctx == NULL)
			klog("early: failed to initialize flanterm");
	}

	klog("early: Hello, World!");
	int *a = pmm_alloc(1);
	klog("test: allocated single page: %p", a);
	*a = 42;
	klog("test: value after write: %d", *a);
	hlt();
}
