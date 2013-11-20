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

#ifndef MY_UBUF_H
#define MY_UBUF_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vector.h"

VECTOR_GENERATE(ubuf, uint8_t);

static inline ubuf *
ubuf_new(void)
{
	return (ubuf_init(64));
}

static inline ubuf *
ubuf_dup_cstr(const char *s)
{
	size_t len = strlen(s);
	ubuf *u = ubuf_init(len + 1);
	ubuf_append(u, (const uint8_t *) s, len);
	return (u);
}

static inline void
ubuf_add_cstr(ubuf *u, const char *s)
{
	if (ubuf_size(u) > 0 && ubuf_value(u, ubuf_size(u) - 1) == '\x00')
		ubuf_clip(u, ubuf_size(u) - 1);
	ubuf_append(u, (const uint8_t *) s, strlen(s));
}

static inline void
ubuf_cterm(ubuf *u)
{
	if (ubuf_size(u) == 0 ||
	    (ubuf_size(u) > 0 && ubuf_value(u, ubuf_size(u) - 1) != '\x00'))
	{
		ubuf_append(u, (const uint8_t *) "\x00", 1);
	}
}

static inline char *
ubuf_cstr(ubuf *u)
{
	ubuf_cterm(u);
	return ((char *) ubuf_data(u));
}

static inline void
ubuf_add_fmt(ubuf *u, const char *fmt, ...)
{
	va_list args, args_copy;
	int status, needed;

	if (ubuf_size(u) > 0 && ubuf_value(u, ubuf_size(u) - 1) == '\x00')
		ubuf_clip(u, ubuf_size(u) - 1);

	va_start(args, fmt);

	va_copy(args_copy, args);
	needed = vsnprintf(NULL, 0, fmt, args_copy);
	assert(needed >= 0);
	va_end(args_copy);

	ubuf_reserve(u, ubuf_size(u) + needed + 1);
	status = vsnprintf((char *) ubuf_ptr(u), needed + 1, fmt, args);
	assert(status >= 0);
	ubuf_advance(u, needed);

	va_end(args);
}

static inline void
ubuf_rstrip(ubuf *u, char s)
{
	if (ubuf_size(u) > 0 &&
	    ubuf_value(u, ubuf_size(u) - 1) == ((uint8_t) s))
	{
		ubuf_clip(u, ubuf_size(u) - 1);
	}
}

#endif /* MY_UBUF_H */
