#include <core/printk.h>
#include <core/time.h>
#include <flanterm.h>
#include <machine/uart.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static struct flanterm_context *g_terminal;

void printk_set_terminal(void *terminal) {
  g_terminal = (struct flanterm_context *)terminal;
}

static void terminal_write_char(char ch) {
  if (g_terminal == NULL) {
    return;
  }

  flanterm_write(g_terminal, &ch, 1);
}

void kputc(char ch) {
  if (ch == '\n') {
    uart_putchar('\r');
    terminal_write_char('\r');
  }

  uart_putchar(ch);
  terminal_write_char(ch);
}

void kputs(const char *str) {
  while (*str != '\0') {
    kputc(*str++);
  }
}

static int print_unsigned(uint64_t value, unsigned int base, bool uppercase) {
  static const char digits_lower[] = "0123456789abcdef";
  static const char digits_upper[] = "0123456789ABCDEF";
  const char *digits = uppercase ? digits_upper : digits_lower;
  char buffer[32];
  int count = 0;
  size_t index = 0;

  if (value == 0) {
    kputc('0');
    return 1;
  }

  while (value != 0) {
    buffer[index++] = digits[value % base];
    value /= base;
  }

  while (index > 0) {
    kputc(buffer[--index]);
    count++;
  }

  return count;
}

static int print_signed(long value) {
  uint64_t magnitude;
  int count = 0;

  if (value < 0) {
    kputc('-');
    count++;
    magnitude = (uint64_t)(-(value + 1)) + 1;
  } else {
    magnitude = (uint64_t)value;
  }

  return count + print_unsigned(magnitude, 10, false);
}

int vprintk(const char *fmt, va_list args) {
  int count = 0;

  while (*fmt != '\0') {
    if (*fmt != '%') {
      kputc(*fmt++);
      count++;
      continue;
    }

    fmt++;

    switch (*fmt) {
    case '\0':
      return count;
    case '%':
      kputc('%');
      count++;
      break;
    case 'c':
      kputc((char)va_arg(args, int));
      count++;
      break;
    case 's': {
      const char *str = va_arg(args, const char *);

      if (str == NULL) {
        str = "(null)";
      }

      while (*str != '\0') {
        kputc(*str++);
        count++;
      }
      break;
    }
    case 'd':
    case 'i':
      count += print_signed(va_arg(args, int));
      break;
    case 'u':
      count += print_unsigned(va_arg(args, unsigned int), 10, false);
      break;
    case 'x':
      count += print_unsigned(va_arg(args, unsigned int), 16, false);
      break;
    case 'X':
      count += print_unsigned(va_arg(args, unsigned int), 16, true);
      break;
    case 'p': {
      uintptr_t value = (uintptr_t)va_arg(args, void *);
      kputs("0x");
      count += 2;
      count += print_unsigned((uint64_t)value, 16, false);
      break;
    }
    default:
      kputc('%');
      kputc(*fmt);
      count += 2;
      break;
    }

    fmt++;
  }

  return count;
}

int kprintf(const char *fmt, ...) {
  va_list args;
  int count;

  va_start(args, fmt);
  count = vprintk(fmt, args);
  va_end(args);

  return count;
}

static void klog_write_timestamp(void) {
  struct time_uptime uptime;

  time_get_uptime(&uptime);

  kputc('[');
  kprintf("%u", (unsigned int)uptime.seconds);
  kputc('.');

  if (uptime.milliseconds < 100) {
    kputc('0');
  }

  if (uptime.milliseconds < 10) {
    kputc('0');
  }

  kprintf("%u", (unsigned int)uptime.milliseconds);
  kputc(']');
  kputc(' ');
}

void klog_impl(const char *ns, const char *fmt, ...) {
  va_list args;

  klog_write_timestamp();
  kprintf("%s: ", ns);

  va_start(args, fmt);
  vprintk(fmt, args);
  va_end(args);

  kputc('\n');
}
