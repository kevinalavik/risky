#ifndef LIB_PRINTK_H
#define LIB_PRINTK_H

#include <stdarg.h>

int vkprintf(const char *fmt, va_list args);
int kprintf(const char *fmt, ...);
int vklog(const char *fmt, va_list args);
int klog(const char *fmt, ...);

#endif // LIB_PRINTK_H
