#include <stddef.h>

void *memset(void *dest, int value, size_t count)
{
    unsigned char *p = dest;
    volatile unsigned char *vp = p;

    while (count--) {
        *vp++ = (unsigned char)value;
    }

    return dest;
}

