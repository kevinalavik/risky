#ifndef ARCH_TIME_H
#define ARCH_TIME_H

#include <sys/types.h>

#define ARCH_TIMER_FREQUENCY_HZ 10000000UL

static inline uint64_t arch_read_time(void)
{
	uint64_t value;

	__asm__ volatile("csrr %0, time" : "=r"(value));
	return value;
}

#endif // ARCH_TIME_H
