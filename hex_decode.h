/*
 * Copyright (c) 2012 by Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.	IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef MY_HEX_DECODE_H
#define MY_HEX_DECODE_H

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "my_alloc.h"

static inline bool
hex_to_int(char hex, uint8_t *val)
{
	hex = toupper(hex);
	switch (hex) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		*val = (hex - '0');
		return (true);
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
		*val = (hex - 55);
		return (true);
	default:
		return (false);
	}
}

static inline bool
hex_decode(const char *hex, uint8_t **raw, size_t *len)
{
	size_t hexlen = strlen(hex);
	uint8_t *p;

	if (hexlen == 0 || (hexlen % 2) != 0)
		return (false);
	*len = hexlen / 2;
	p = *raw = my_malloc(*len);
	while (hexlen != 0) {
		uint8_t val[2];

		if (!hex_to_int(*hex, &val[0]))
			goto err;
		hex++;
		if (!hex_to_int(*hex, &val[1]))
			goto err;
		hex++;

		*p = (val[0] << 4) | val[1];
		p++;

		hexlen -= 2;
	}
	return (true);
err:
	free(*raw);
	return (false);
}

#endif /* MY_HEX_DECODE_H */
