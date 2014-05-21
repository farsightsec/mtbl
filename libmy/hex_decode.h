/*
 * Copyright (c) 2012 by Farsight Security, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
	my_free(*raw);
	return (false);
}

#endif /* MY_HEX_DECODE_H */
