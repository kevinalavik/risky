#ifndef BOOT_REQ_H
#define BOOT_REQ_H

#include <limine.h>

extern volatile uint64_t limine_base_revision[];
extern volatile struct limine_framebuffer_request framebuffer_request;
extern volatile struct limine_dtb_request dtb_request;
extern volatile struct limine_hhdm_request hhdm_request;
extern volatile struct limine_executable_address_request
	executable_address_request;

#endif // BOOT_REQ_H