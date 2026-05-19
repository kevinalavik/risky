#include <string.h>

void *memcpy(void *dest, const void *src, size_t count)
{
	unsigned char *dest_bytes = (unsigned char *)dest;
	const unsigned char *src_bytes = (const unsigned char *)src;

	for (size_t i = 0; i < count; i++) {
		dest_bytes[i] = src_bytes[i];
	}

	return dest;
}

void *memset(void *dest, int value, size_t count)
{
	unsigned char *dest_bytes = (unsigned char *)dest;

	for (size_t i = 0; i < count; i++) {
		dest_bytes[i] = (unsigned char)value;
	}

	return dest;
}
