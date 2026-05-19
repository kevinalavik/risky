#ifndef LIB_TIME_H
#define LIB_TIME_H

#include <stdbool.h>
#include <stdint.h>

bool ktime_init(void);
uint64_t ktime_get_ticks(void);
uint32_t ktime_get_freq(void);
uint64_t ktime_get_ms(void);

#endif // LIB_TIME_H
