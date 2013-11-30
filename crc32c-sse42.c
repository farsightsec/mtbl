/*
 * Copyright (c) 2013 by Farsight Security, Inc.
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
Copyright (c) 2008,2009,2010 Massachusetts Institute of Technology.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

* Neither the name of the Massachusetts Institute of Technology nor
  the names of its contributors may be used to endorse or promote
  products derived from this software without specific prior written
  permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * Copyright (c) 2004-2006 Intel Corporation - All Rights Reserved
 *
 * This software program is licensed subject to the BSD License, 
 * available at http://www.opensource.org/licenses/bsd-license.html
 */

// Copyright 2010 Google Inc.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#if __GNUC__ >= 3 && defined(__x86_64__)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CPUID_FEATURES		(1)
#define SSE42_FEATURE_BIT	(1 << 20)

bool my_crc32c_sse42_supported(void);

uint32_t my_crc32c_sse42(const uint8_t *, size_t);

static uint32_t
cpuid(uint32_t level)
{
	uint32_t eax, ebx, ecx, edx;
	asm("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (level));
	return (ecx);
}

bool
my_crc32c_sse42_supported(void)
{
	return (cpuid(CPUID_FEATURES) & SSE42_FEATURE_BIT);
}

static inline uint64_t
my_asm_crc32_u64(uint64_t crc, uint64_t value) {
	asm("crc32q %[value], %[crc]\n" : [crc] "+r" (crc) : [value] "rm" (value));
	return crc;
}

static inline uint32_t
my_asm_crc32_u32(uint32_t crc, uint32_t value) {
	asm("crc32l %[value], %[crc]\n" : [crc] "+r" (crc) : [value] "rm" (value));
	return crc;
}

static inline uint32_t
my_asm_crc32_u16(uint32_t crc, uint16_t value) {
	asm("crc32w %[value], %[crc]\n" : [crc] "+r" (crc) : [value] "rm" (value));
	return crc;
}

static inline uint32_t
my_asm_crc32_u8(uint32_t crc, uint8_t value) {
	asm("crc32b %[value], %[crc]\n" : [crc] "+r" (crc) : [value] "rm" (value));
	return crc;
}

uint32_t
my_crc32c_sse42(const uint8_t *buf, size_t len)
{
	const uint8_t *p = buf;
	uint64_t crc64bit = 0xFFFFFFFF;

	for (size_t i = 0; i < len / sizeof(uint64_t); i++) {
		crc64bit = my_asm_crc32_u64(crc64bit, *(uint64_t *) p);
		p += sizeof(uint64_t);
	}

	uint32_t crc32bit = (uint32_t) crc64bit;
	len &= sizeof(uint64_t) - 1;
	switch (len) {
	case 7:
		crc32bit = my_asm_crc32_u8(crc32bit, *p++);
	case 6:
		crc32bit = my_asm_crc32_u16(crc32bit, *(uint16_t *) p);
		p += 2;
	case 4:
		crc32bit = my_asm_crc32_u32(crc32bit, *(uint32_t *) p);
		break;
	case 3:
		crc32bit = my_asm_crc32_u8(crc32bit, *p++);
	case 2:
		crc32bit = my_asm_crc32_u16(crc32bit, *(uint16_t *) p);
		break;
	case 5:
		crc32bit = my_asm_crc32_u32(crc32bit, *(uint32_t *) p);
		p += 4;
	case 1:
		crc32bit = my_asm_crc32_u8(crc32bit, *p);
		break;
	case 0:
		break;
	default:
		break;
	}

	return ~crc32bit;
}

#endif /* __GNUC__ >= 3 && defined(__x86_64__) */
