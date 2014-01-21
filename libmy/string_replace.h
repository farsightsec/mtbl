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

#ifndef MY_STRING_REPLACE_H
#define MY_STRING_REPLACE_H

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

#endif /* MY_STRING_REPLACE_H */
