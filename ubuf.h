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

#ifndef RSF_UBUF_H
#define RSF_UBUF_H

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

#endif /* RSF_UBUF_H */
