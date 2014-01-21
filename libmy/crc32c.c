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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <stdio.h>

#include "crc32c.h"

static uint32_t my_crc32c_first(const uint8_t *buf, size_t len);

my_crc32c_fp my_crc32c = my_crc32c_first;

/* crc32c-slicing.c */
uint32_t my_crc32c_slicing(const uint8_t *, size_t);

/* crc32c-sse42.c */
#if __GNUC__ >= 3 && defined(__x86_64__)
bool my_crc32c_sse42_supported(void);
uint32_t my_crc32c_sse42(const uint8_t *, size_t);
#endif

#if defined(__GNUC__)
__attribute__((constructor))
#endif
static void
my_crc32c_runtime_detection(void)
{
#if __GNUC__ >= 3 && defined(__x86_64__)
	if (my_crc32c_sse42_supported()) {
		my_crc32c = my_crc32c_sse42;
	} else {
		my_crc32c = my_crc32c_slicing;
	}
#else
	my_crc32c = my_crc32c_slicing;
#endif
}

static uint32_t
my_crc32c_first(const uint8_t *buf, size_t len) {
	my_crc32c_runtime_detection();
	return my_crc32c(buf, len);
}
