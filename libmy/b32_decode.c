/*
 * Copyright (c) 2015 by Farsight Security, Inc.
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
 *     http://tools.ietf.org/html/rfc4648
 */

#include <stdint.h>
#include <string.h>

#include "b32_decode.h"

#ifndef G_N_ELEMENTS
#define G_N_ELEMENTS(arr) (sizeof (arr) / sizeof ((arr)[0]))
#endif

#define ZERO(x)         memset((x), 0, sizeof *(x))

static inline void *
ptr_add_offset(void *p, size_t offset)
{
        /* Using size_t instead of 'char *' because pointers don't wrap. */
        return (void *) ((size_t) p + offset);
}

static inline const void *
cast_to_constpointer(const void *p)
{
        return p;
}

static inline bool
is_ascii_lower(int c)
{
        return c >= 97 && c <= 122;             /* a-z */
}

static inline int
ascii_toupper(int c)
{
        return is_ascii_lower(c) ? c - 32 : c;
}

static inline size_t
ptr_diff(const void *a, const void *b)
{
        return (const char *) a - (const char *) b;
}

static const char base32_alphabet[32] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V'
};

static char base32_map[(size_t) (unsigned char) -1 + 1];

/**
 * Decode a base32 encoding of `len' bytes of `data' into the buffer `dst'.
 *
 * @param dst		destination buffer
 * @param size		length of destination
 * @param data		start of data to decode
 * @param len		amount of encoded data to decode
 *
 * @return the amount of bytes decoded into the destination.
 */
size_t
base32_decode(void *dst, size_t size, const char *data, size_t len)
{
	const char *end = ptr_add_offset(dst, size);
	const unsigned char *p = cast_to_constpointer(data);
	char s[8];
	char *q = dst;
	int pad = 0;
	size_t i, si;

	if (0 == base32_map[0]) {
		for (i = 0; i < G_N_ELEMENTS(base32_map); i++) {
			const char *x;

			x = memchr(base32_alphabet, ascii_toupper(i),
					   sizeof base32_alphabet);
			base32_map[i] = x ? (x - base32_alphabet) : (unsigned char) -1;
		}
	}

	ZERO(&s);
	si = 0;
	i = 0;

	while (i < len) {
		unsigned char c;

		c = p[i++];
		if ('=' == c) {
			pad++;
			c = 0;
		} else {
			c = base32_map[c];
			if ((unsigned char) -1 == c) {
				return -1;
			}
		}

		s[si++] = c;

		if (G_N_ELEMENTS(s) == si || pad > 0 || i == len) {
			char b[5];
			size_t bi;

			memset(&s[si], 0, G_N_ELEMENTS(s) - si);
			si = 0;

			b[0] =
				((s[0] << 3) & 0xf8) |
				((s[1] >> 2) & 0x07);
			b[1] =
				((s[1] & 0x03) << 6) |
				((s[2] & 0x1f) << 1) |
				((s[3] >> 4) & 1);
			b[2] =
				((s[3] & 0x0f) << 4) |
				((s[4] >> 1) & 0x0f);
			b[3] =
				((s[4] & 1) << 7) |
				((s[5] & 0x1f) << 2) |
				((s[6] >> 3) & 0x03);
			b[4] =
				((s[6] & 0x07) << 5) |
				(s[7] & 0x1f);

			for (bi = 0; bi < G_N_ELEMENTS(b) && q != end; bi++) {
				*q++ = b[bi];
			}
		}

		if (end == q) {
			break;
		}
	}

	return ptr_diff(q, dst);
}
