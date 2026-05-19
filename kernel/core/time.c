#include <arch/time.h>
#include <core/time.h>

static uint64_t g_timebase_frequency = ARCH_TIMER_FREQUENCY_HZ;
static uint64_t g_boot_ticks;

void time_init(void)
{
	const struct fdt *fdt = fdt_get();
	struct fdt_node cpus;
	uint32_t frequency = 0;

	if (fdt != 0 && fdt_find_path(fdt, "/cpus", &cpus) &&
		fdt_read_u32(&cpus, "timebase-frequency", &frequency) &&
		frequency != 0) {
		g_timebase_frequency = (uint64_t)frequency;
	}

	g_boot_ticks = arch_read_time();
}

uint64_t time_now_ticks(void)
{
	return arch_read_time() - g_boot_ticks;
}

uint64_t time_frequency_hz(void)
{
	return g_timebase_frequency;
}

void time_get_uptime(struct time_uptime *uptime)
{
	uint64_t ticks;

	if (uptime == NULL) {
		return;
	}

	ticks = time_now_ticks();
	uptime->seconds = ticks / g_timebase_frequency;
	uptime->milliseconds = (uint32_t)((ticks % g_timebase_frequency) * 1000UL /
									  g_timebase_frequency);
}
