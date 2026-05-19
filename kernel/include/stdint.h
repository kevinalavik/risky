#ifndef STDINT_H
#define STDINT_H

#include <sys/types.h>

typedef unsigned long uintptr_t;
typedef long intptr_t;

#define UINT8_MAX ((uint8_t)0xffU)
#define UINT16_MAX ((uint16_t)0xffffU)
#define UINT32_MAX ((uint32_t)0xffffffffU)
#define UINT64_MAX ((uint64_t)0xffffffffffffffffUL)
#define SIZE_MAX ((size_t)-1)

#endif // STDINT_H
