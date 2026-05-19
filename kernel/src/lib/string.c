#include <stdint.h>
#include <stddef.h>
#include <lib/string.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n)
{
	uint8_t *restrict pdest = dest;
	const uint8_t *restrict psrc = src;

	for (size_t i = 0; i < n; i++) {
		pdest[i] = psrc[i];
	}

	return dest;
}

void *memset(void *s, int c, size_t n)
{
	uint8_t *p = s;

	for (size_t i = 0; i < n; i++) {
		p[i] = (uint8_t)c;
	}

	return s;
}

void *memmove(void *dest, const void *src, size_t n)
{
	uint8_t *pdest = dest;
	const uint8_t *psrc = src;

	if ((uintptr_t)src > (uintptr_t)dest) {
		for (size_t i = 0; i < n; i++) {
			pdest[i] = psrc[i];
		}
	} else if ((uintptr_t)src < (uintptr_t)dest) {
		for (size_t i = n; i > 0; i--) {
			pdest[i - 1] = psrc[i - 1];
		}
	}

	return dest;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
	const uint8_t *p1 = s1;
	const uint8_t *p2 = s2;

	for (size_t i = 0; i < n; i++) {
		if (p1[i] != p2[i]) {
			return p1[i] < p2[i] ? -1 : 1;
		}
	}

	return 0;
}

size_t strlen(const char *s)
{
	size_t len = 0;

	while (s[len] != '\0') {
		len++;
	}

	return len;
}

size_t strnlen(const char *s, size_t maxlen)
{
	size_t len = 0;

	while (len < maxlen && s[len] != '\0') {
		len++;
	}

	return len;
}

int strcmp(const char *s1, const char *s2)
{
	size_t i = 0;

	while (s1[i] != '\0' && s2[i] != '\0') {
		if (s1[i] != s2[i]) {
			return (unsigned char)s1[i] < (unsigned char)s2[i] ? -1 : 1;
		}

		i++;
	}

	if (s1[i] == s2[i]) {
		return 0;
	}

	return s1[i] == '\0' ? -1 : 1;
}

const char *strchr(const char *s, int c)
{
	char needle = (char)c;

	for (;;) {
		if (*s == needle) {
			return s;
		}

		if (*s == '\0') {
			return NULL;
		}

		s++;
	}
}
