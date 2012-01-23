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

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/*
 * Copyright 2008, Dave Benson.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You may
 * obtain a copy of the License at
 *
 *	http://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "mtbl-private.h"

unsigned
mtbl_varint_length(uint64_t v)
{
	unsigned len = 1;
	while (v >= 128) {
		v >>= 7;
		len++;
	}
	return (len);
}

unsigned
mtbl_varint_length_packed(const uint8_t *data)
{
	unsigned i;
	for (i = 0; i < 10; i++)
		if ((data[i] & 0x80) == 0)
			break;
	if (i == 10)
		return (0);
	return (i + 1);
}

void
mtbl_varint_encode32(uint8_t **pptr, uint32_t v)
{
	static const unsigned B = 128;
	uint8_t *ptr = *pptr;
	if (v < (1 << 7)) {
		*(ptr++) = v;
	} else if (v < (1 << 14)) {
		*(ptr++) = v | B;
		*(ptr++) = v >> 7;
	} else if (v < (1 << 21)) {
		*(ptr++) = v | B;
		*(ptr++) = (v >> 7) | B;
		*(ptr++) = v >> 14;
	} else if (v < (1 << 28)) {
		*(ptr++) = v | B;
		*(ptr++) = (v >> 7) | B;
		*(ptr++) = (v >> 14) | B;
		*(ptr++) = v >> 21;
	} else {
		*(ptr++) = v | B;
		*(ptr++) = (v >> 7) | B;
		*(ptr++) = (v >> 14) | B;
		*(ptr++) = (v >> 21) | B;
		*(ptr++) = v >> 28;
	}
	*pptr = ptr;
}

void
mtbl_varint_encode64(uint8_t **pptr, uint64_t v)
{
	static const unsigned B = 128;
	uint8_t *ptr = *pptr;
	while (v >= B) {
		*(ptr++) = (v & (B - 1)) | B;
		v >>= 7;
	}
	*(ptr++) = (uint8_t) v;
	*pptr = ptr;
}

uint32_t
mtbl_varint_decode32(uint8_t **ptr)
{
	uint8_t *data = *ptr;
	unsigned len = mtbl_varint_length_packed(*ptr);
	uint32_t val = data[0] & 0x7f;
	if (len > 1) {
		val |= ((data[1] & 0x7f) << 7);
		if (len > 2) {
			val |= ((data[2] & 0x7f) << 14);
			if (len > 3) {
				val |= ((data[3] & 0x7f) << 21);
				if (len > 4)
					val |= (data[4] << 28);
			}
		}
	}
	*ptr = data + len;
	return (val);
}

uint64_t
mtbl_varint_decode64(uint8_t **ptr) {
	uint8_t *data = *ptr;
	unsigned shift, i;
	unsigned len = mtbl_varint_length_packed(*ptr);
	uint64_t val;
	if (len < 5)
		return (mtbl_varint_decode32(ptr));
	val = ((data[0] & 0x7f))
		| ((data[1] & 0x7f) << 7)
		| ((data[2] & 0x7f) << 14)
		| ((data[3] & 0x7f) << 21);
	shift = 28;
	for (i = 4; i < len; i++) {
		val |= (((uint64_t)(data[i] & 0x7f)) << shift);
		shift += 7;
	}
	*ptr = data + len;
	return (val);
}
