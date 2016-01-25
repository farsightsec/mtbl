/*
 * Copyright (c) 2010 by Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright (c) 2006 Christian Biere <christianbiere@gmx.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the authors nor the names of its contributors
 *	may be used to endorse or promote products derived from this software
 *	without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * See RFC 4648 for details about Base 32 hex encoding:
 *	http://tools.ietf.org/html/rfc4648
 */

#include <stdint.h>
#include <string.h>

#include "b32_encode.h"

static const char base32_alphabet[32] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V'
};

/**
 * Encode in base32 `len' bytes of `data' into the buffer `dst'.
 *
 * @param dst		destination buffer
 * @param size		length of destination
 * @param data		start of data to encode
 * @param len		amount of bytes to encode
 *
 * @return the amount of bytes generated into the destination.
 */
size_t
base32_encode(char *dst, size_t size, const void *data, size_t len)
{
	size_t i = 0;
	const uint8_t *p = data;
	const char *end = &dst[size];
	char *q = dst;

	do {
		size_t j, k;
		uint8_t x[5];
		char s[8];

		switch (len - i) {
		case 4:
			k = 7;
			break;
		case 3:
			k = 5;
			break;
		case 2:
			k = 3;
			break;
		case 1:
			k = 2;
			break;
		default:
			k = 8;
		}

		for (j = 0; j < 5; j++)
			x[j] = i < len ? p[i++] : 0;

		s[0] = (x[0] >> 3);
		s[1] = ((x[0] & 0x07) << 2) | (x[1] >> 6);
		s[2] = (x[1] >> 1) & 0x1f;
		s[3] = ((x[1] & 0x01) << 4) | (x[2] >> 4);
		s[4] = ((x[2] & 0x0f) << 1) | (x[3] >> 7);
		s[5] = (x[3] >> 2) & 0x1f;
		s[6] = ((x[3] & 0x03) << 3) | (x[4] >> 5);
		s[7] = x[4] & 0x1f;

		for (j = 0; j < k && q != end; j++) {
			*q++ = base32_alphabet[(uint8_t) s[j]];
		}

		if (end == q) {
			break;
		}

	} while (i < len);

	return q - dst;
}
