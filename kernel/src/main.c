#include <stdbool.h>
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

static struct flanterm_context *g_ft_ctx;

void printk_putc(char c)
{
	uart_putc(c);

	if (g_ft_ctx != NULL)
		flanterm_write(g_ft_ctx, &c, 1);
}

void kmain(void)
{
	if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision)) {
		hlt();
	}

	if (hhdm_request.response != NULL &&
		executable_address_request.response != NULL) {
		cpu_init_mappings(hhdm_request.response->offset,
						  executable_address_request.response->physical_base,
						  executable_address_request.response->virtual_base);
	}

	dtb_init(dtb_request.response);
	uart_init();

	if (framebuffer_request.response == NULL ||
		framebuffer_request.response->framebuffer_count < 1) {
		klog("early: failed to get framebuffer, no ramfb?\n");
	} else {
		struct limine_framebuffer *fb =
			framebuffer_request.response->framebuffers[0];

		g_ft_ctx = flanterm_fb_init(
			NULL, NULL, fb->address, fb->width, fb->height, fb->pitch,
			fb->red_mask_size, fb->red_mask_shift, fb->green_mask_size,
			fb->green_mask_shift, fb->blue_mask_size, fb->blue_mask_shift, NULL,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 1, 0, 0, 0, 0);

		if (g_ft_ctx == NULL)
			klog("early: failed to initialize flanterm\n");
	}

	klog("early: Hello, World!\n");
	hlt();
}
