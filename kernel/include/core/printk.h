#ifndef CORE_PRINTK_H
#define CORE_PRINTK_H

#include <stdarg.h>

#ifndef KLOG_NS
#define KLOG_NS "kernel"
#endif

void printk_set_terminal(void *terminal);
void kputc(char ch);
void kputs(const char *str);
int vprintk(const char *fmt, va_list args);
int kprintf(const char *fmt, ...);
void klog_impl(const char *ns, const char *fmt, ...);

#define klog(msg, ...) klog_impl(KLOG_NS, msg, ##__VA_ARGS__)

#endif // CORE_PRINTK_H
