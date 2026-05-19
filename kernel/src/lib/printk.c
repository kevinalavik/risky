#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <lib/printk.h>
#include <lib/time.h>

extern void printk_putc(char c);

enum printk_length {
	PRINTK_LEN_DEFAULT,
	PRINTK_LEN_CHAR,
	PRINTK_LEN_SHORT,
	PRINTK_LEN_LONG,
	PRINTK_LEN_LLONG,
	PRINTK_LEN_SIZE,
	PRINTK_LEN_PTRDIFF,
};

struct printk_state {
	bool prefix_enabled;
	bool at_line_start;
	int written;
};

static void emit_unsigned(struct printk_state *state, uint64_t value,
						  unsigned int base, bool uppercase);
static void emit_raw_unsigned(struct printk_state *state, uint64_t value,
							  unsigned int base, bool uppercase);

static void emit_raw_char(struct printk_state *state, char c)
{
	if (c == '\n') {
		printk_putc('\r');
		printk_putc('\n');
		state->written += 2;
		state->at_line_start = true;
		return;
	}

	printk_putc(c);
	state->written++;
}

static void emit_log_prefix(struct printk_state *state)
{
	uint64_t uptime_ms;
	uint64_t seconds;
	uint64_t millis;

	if (!state->prefix_enabled || !state->at_line_start)
		return;

	uptime_ms = ktime_get_ms();
	seconds = uptime_ms / 1000U;
	millis = uptime_ms % 1000U;

	emit_raw_char(state, '[');
	emit_raw_unsigned(state, seconds, 10U, false);
	emit_raw_char(state, '.');
	emit_raw_char(state, (char)('0' + (millis / 100U) % 10U));
	emit_raw_char(state, (char)('0' + (millis / 10U) % 10U));
	emit_raw_char(state, (char)('0' + millis % 10U));
	emit_raw_char(state, ']');
	emit_raw_char(state, ' ');
	state->at_line_start = false;
}

static void emit_char(struct printk_state *state, char c)
{
	emit_log_prefix(state);
	emit_raw_char(state, c);

	if (c != '\n')
		state->at_line_start = false;
}

static void emit_string(struct printk_state *state, const char *s)
{
	if (s == NULL)
		s = "(null)";

	while (*s != '\0')
		emit_char(state, *s++);
}

static void emit_unsigned(struct printk_state *state, uint64_t value,
						  unsigned int base, bool uppercase)
{
	emit_raw_unsigned(state, value, base, uppercase);
}

static void emit_raw_unsigned(struct printk_state *state, uint64_t value,
							  unsigned int base, bool uppercase)
{
	char buffer[sizeof(uint64_t) * 8];
	const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
	size_t len = 0;

	if (base < 2U || base > 16U)
		return;

	do {
		buffer[len++] = digits[value % base];
		value /= base;
	} while (value != 0U);

	while (len > 0)
		emit_raw_char(state, buffer[--len]);
}

static uint64_t get_unsigned_arg(va_list *args, enum printk_length length)
{
	switch (length) {
	case PRINTK_LEN_CHAR:
		return (unsigned char)va_arg(*args, unsigned int);
	case PRINTK_LEN_SHORT:
		return (unsigned short)va_arg(*args, unsigned int);
	case PRINTK_LEN_LONG:
		return va_arg(*args, unsigned long);
	case PRINTK_LEN_LLONG:
		return va_arg(*args, unsigned long long);
	case PRINTK_LEN_SIZE:
		return va_arg(*args, size_t);
	case PRINTK_LEN_PTRDIFF:
		return (uint64_t)va_arg(*args, ptrdiff_t);
	case PRINTK_LEN_DEFAULT:
	default:
		return va_arg(*args, unsigned int);
	}
}

static int64_t get_signed_arg(va_list *args, enum printk_length length)
{
	switch (length) {
	case PRINTK_LEN_CHAR:
		return (signed char)va_arg(*args, int);
	case PRINTK_LEN_SHORT:
		return (short)va_arg(*args, int);
	case PRINTK_LEN_LONG:
		return va_arg(*args, long);
	case PRINTK_LEN_LLONG:
		return va_arg(*args, long long);
	case PRINTK_LEN_SIZE:
		return (int64_t)va_arg(*args, ptrdiff_t);
	case PRINTK_LEN_PTRDIFF:
		return va_arg(*args, ptrdiff_t);
	case PRINTK_LEN_DEFAULT:
	default:
		return va_arg(*args, int);
	}
}

static int vprintk_internal(bool prefix_enabled, const char *fmt, va_list args)
{
	struct printk_state state = {
		.prefix_enabled = prefix_enabled,
		.at_line_start = true,
		.written = 0,
	};

	while (*fmt != '\0') {
		enum printk_length length = PRINTK_LEN_DEFAULT;

		if (*fmt != '%') {
			emit_char(&state, *fmt++);
			continue;
		}

		fmt++;
		if (*fmt == '\0')
			break;

		if (*fmt == 'h') {
			fmt++;
			if (*fmt == 'h') {
				length = PRINTK_LEN_CHAR;
				fmt++;
			} else {
				length = PRINTK_LEN_SHORT;
			}
		} else if (*fmt == 'l') {
			fmt++;
			if (*fmt == 'l') {
				length = PRINTK_LEN_LLONG;
				fmt++;
			} else {
				length = PRINTK_LEN_LONG;
			}
		} else if (*fmt == 'z') {
			length = PRINTK_LEN_SIZE;
			fmt++;
		} else if (*fmt == 't') {
			length = PRINTK_LEN_PTRDIFF;
			fmt++;
		}

		switch (*fmt) {
		case '%':
			emit_char(&state, '%');
			fmt++;
			break;
		case 'c':
			emit_char(&state, (char)va_arg(args, int));
			fmt++;
			break;
		case 's':
			emit_string(&state, va_arg(args, const char *));
			fmt++;
			break;
		case 'd':
		case 'i': {
			int64_t value = get_signed_arg(&args, length);

			if (value < 0) {
				uint64_t magnitude = (uint64_t)(-(value + 1)) + 1U;
				emit_char(&state, '-');
				emit_unsigned(&state, magnitude, 10U, false);
			} else {
				emit_unsigned(&state, (uint64_t)value, 10U, false);
			}

			fmt++;
			break;
		}
		case 'u':
			emit_unsigned(&state, get_unsigned_arg(&args, length), 10U, false);
			fmt++;
			break;
		case 'x':
			emit_unsigned(&state, get_unsigned_arg(&args, length), 16U, false);
			fmt++;
			break;
		case 'X':
			emit_unsigned(&state, get_unsigned_arg(&args, length), 16U, true);
			fmt++;
			break;
		case 'p': {
			uintptr_t value = (uintptr_t)va_arg(args, void *);

			emit_string(&state, "0x");
			emit_unsigned(&state, (uint64_t)value, 16U, false);
			fmt++;
			break;
		}
		default:
			emit_char(&state, '%');
			emit_char(&state, *fmt++);
			break;
		}
	}

	return state.written;
}

int vkprintf(const char *fmt, va_list args)
{
	return vprintk_internal(false, fmt, args);
}

int kprintf(const char *fmt, ...)
{
	va_list args;
	int written;

	va_start(args, fmt);
	written = vkprintf(fmt, args);
	va_end(args);

	return written;
}

int vklog(const char *fmt, va_list args)
{
	return vprintk_internal(true, fmt, args) + kprintf("\n");
}

int klog(const char *fmt, ...)
{
	va_list args;
	int written;

	va_start(args, fmt);
	written = vklog(fmt, args);
	va_end(args);

	return written;
}
