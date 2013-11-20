/*
 * Copyright (c) 2012, 2013 by Farsight Security, Inc.
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
 * Copyright (c) 2008, Dave Benson.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "varint.h"

unsigned
varint_length(uint64_t v)
{
	unsigned len = 1;
	while (v >= 128) {
		v >>= 7;
		len++;
	}
	return (len);
}

unsigned
varint_length_packed(const uint8_t *data, size_t len_data)
{
	unsigned i = 0;
	size_t len = len_data;
	while (len--) {
		if ((data[i] & 0x80) == 0)
			break;
		i++;
	}
	if (i == len_data)
		return (0);
	return (i + 1);
}

size_t
varint_encode32(uint8_t *src_ptr, uint32_t v)
{
	static const unsigned B = 128;
	uint8_t *ptr = src_ptr;
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
	return ((size_t) (ptr - src_ptr));
}

size_t
varint_encode64(uint8_t *src_ptr, uint64_t v)
{
	static const unsigned B = 128;
	uint8_t *ptr = src_ptr;
	while (v >= B) {
		*(ptr++) = (v & (B - 1)) | B;
		v >>= 7;
	}
	*(ptr++) = (uint8_t) v;
	return ((size_t) (ptr - src_ptr));
}

size_t
varint_decode32(const uint8_t *data, uint32_t *value)
{
	unsigned len = varint_length_packed(data, 5);
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
	*value = val;
	return ((size_t) len);
}

size_t
varint_decode64(const uint8_t *data, uint64_t *value)
{
	unsigned shift, i;
	unsigned len = varint_length_packed(data, 10);
	uint64_t val;
	if (len < 5) {
		size_t tmp_len;
		uint32_t tmp;
		tmp_len = varint_decode32(data, &tmp);
		*value = tmp;
		return (tmp_len);
	}
	val = ((data[0] & 0x7f))
		| ((data[1] & 0x7f) << 7)
		| ((data[2] & 0x7f) << 14)
		| ((data[3] & 0x7f) << 21);
	shift = 28;
	for (i = 4; i < len; i++) {
		val |= (((uint64_t)(data[i] & 0x7f)) << shift);
		shift += 7;
	}
	*value = val;
	return ((size_t) len);
}
