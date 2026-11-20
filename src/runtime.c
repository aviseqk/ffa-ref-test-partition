#include <stddef.h>
#include <stdint.h>

#include "ffa/ffa_abi.h"

void *memset(void *dest, int value, size_t count)
{
    unsigned char *p = dest;
    volatile unsigned char *vp = p;

    while (count--) {
        *vp++ = (unsigned char)value;
    }

    return dest;
}

void print_hex16(uint16_t value)
{
	static const char hex[] = "0123456789abcdef";

	char buffer[8];

	buffer[0] = '0';
	buffer[1] = 'x';

	for (unsigned int i = 0; i < 4; i++) {
		const unsigned int shift = (3 - i) * 4;
		buffer[2 + i] = hex[(value >> shift) & 0xf];
	}

	buffer[6] = '\r';
	buffer[7] = '\n';

	char output[9];

	for (unsigned int i = 0; i < 8; i++) {
		output[i] = buffer[i];
	}


	output[8] = '\0';

	(void)ffa_console_puts(output);
}

void print_hex32(uint32_t value)
{
	static const char hex[] = "0123456789abcdef";

	char buffer[12];

	buffer[0] = '0';
	buffer[1] = 'x';

	for (unsigned int i = 0; i < 8; i++) {
		const unsigned int shift = (7 - i) * 4;
		buffer[2 + i] = hex[(value >> shift) & 0xf];
	}

	buffer[10] = '\r';
	buffer[11] = '\n';

	char output[13];

	for (unsigned int i = 0; i < 12; i++) {
		output[i] = buffer[i];
	}


	output[12] = '\0';

	(void)ffa_console_puts(output);
}

void print_hex64(uint64_t value)
{
	static const char hex[] = "0123456789abcdef";

	char output[22];

	output[0] = '0';
	output[1] = 'x';

	for (unsigned int i = 0; i < 16; i++) {
		const unsigned int shift = (15 - i) * 4;
		output[2 + i] = hex[(value >> shift) & 0xf];
	}

	output[18] = '\r';
	output[19] = '\n';
	output[20] = '\0';

	(void)ffa_console_puts(output);
}
