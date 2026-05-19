#include <stdbool.h>
#include <stdint.h>
#include <cpu.h>
#include <dev/dtb.h>
#include <lib/time.h>

#define KTIME_FALLBACK_FREQ 10000000U

static uint64_t ktime_boot_ticks;
static uint32_t ktime_tick_freq;
static bool ktime_ready;

bool ktime_init(void)
{
	struct dtb_node cpus;
	uint32_t dtb_freq;

	ktime_tick_freq = KTIME_FALLBACK_FREQ;
	if (dtb_find_node_by_path("/cpus", &cpus) &&
		dtb_get_u32(&cpus, "timebase-frequency", &dtb_freq) && dtb_freq != 0) {
		ktime_tick_freq = dtb_freq;
	}

	ktime_boot_ticks = cpu_read_time();
	ktime_ready = true;
	return true;
}

uint64_t ktime_get_ticks(void)
{
	if (!ktime_ready)
		return 0;

	return cpu_read_time() - ktime_boot_ticks;
}

uint32_t ktime_get_freq(void)
{
	return ktime_tick_freq;
}

uint64_t ktime_get_ms(void)
{
	uint32_t freq = ktime_get_freq();
	uint64_t ticks = ktime_get_ticks();

	if (freq == 0)
		return 0;

	return (ticks * 1000U) / freq;
}
