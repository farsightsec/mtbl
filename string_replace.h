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

#ifndef RSF_STRING_REPLACE_H
#define RSF_STRING_REPLACE_H

#include <string.h>

#include "ubuf.h"

static inline char *
string_replace(const char *str, const char *old, const char *new)
{
	const char *end;
	char *ret;
	size_t retsz;

	if (strstr(str, old) == NULL) {
		char *s = strdup(str);
		assert(s != NULL);
		return (s);
	}
	ubuf *u = ubuf_new();

	end = str + strlen(str) + 1;
	while ((ret = strstr(str, old)) != NULL) {
		size_t offset = (size_t) (ret - str);
		ubuf_append(u, (uint8_t *) str, offset);
		ubuf_append(u, (uint8_t *) new, strlen(new));
		str += offset;
		if (str + strlen(old) >= end)
			break;
		str += strlen(old);
	}
	ubuf_append(u, (uint8_t *) str, strlen(str));

	ubuf_cterm(u);
	ubuf_detach(u, (uint8_t **) &ret, &retsz);
	ubuf_destroy(&u);
	return (ret);
}

#endif /* RSF_STRING_REPLACE_H */
