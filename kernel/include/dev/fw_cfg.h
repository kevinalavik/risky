#ifndef DEV_FW_CFG_H
#define DEV_FW_CFG_H

#include <sys/types.h>

int fw_cfg_find_file(const char *name, uint16_t *selector, uint32_t *size);
int fw_cfg_dma_write(uint16_t selector, const void *data, uint32_t length);

#endif // DEV_FW_CFG_H
