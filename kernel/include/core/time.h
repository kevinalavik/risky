#ifndef CORE_TIME_H
#define CORE_TIME_H

#include <dt/fdt.h>
#include <sys/types.h>

struct time_uptime {
  uint64_t seconds;
  uint32_t milliseconds;
};

void time_init(const struct fdt *fdt);
uint64_t time_now_ticks(void);
uint64_t time_frequency_hz(void);
void time_get_uptime(struct time_uptime *uptime);

#endif // CORE_TIME_H
