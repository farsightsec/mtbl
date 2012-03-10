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

#include <assert.h>

#include "my_alloc.h"

#define VECTOR_GENERATE(name, type)					\
typedef struct name##__vector {						\
	type *		_v;						\
	type *		_p;						\
	size_t		_n, _n_alloced, _hint;				\
} name;									\
static inline name *							\
name##_init(unsigned hint)						\
{									\
	name *vec;							\
	vec = my_calloc(1, sizeof(name));				\
	if (hint == 0) hint = 1;					\
	vec->_hint = vec->_n_alloced = hint;				\
	vec->_v = my_malloc(vec->_n_alloced * sizeof(type));		\
	vec->_p = &(vec->_v[0]);					\
	return (vec);							\
}									\
static inline void							\
name##_detach(name *vec, type **out, size_t *outsz)			\
{									\
	*(out) = (vec)->_v;						\
	*(outsz) = (vec)->_n;						\
	(vec)->_n_alloced = (vec)->_hint;				\
	(vec)->_v = my_malloc((vec)->_n_alloced * sizeof(type));	\
	(vec)->_p = &(vec->_v[0]);					\
}									\
static inline void							\
name##_destroy(name **vec)						\
{									\
	if (*vec) {							\
		free((*vec)->_v);					\
		free((*vec));						\
		*vec = NULL;						\
	}								\
}									\
static inline void							\
name##_reserve(name *vec, size_t n_elems)				\
{									\
	while ((n_elems) > ((vec)->_n_alloced - (vec)->_n)) {		\
		(vec)->_n_alloced *= 2;					\
		(vec)->_v = my_realloc((vec)->_v, (vec)->_n_alloced	\
				   * sizeof(type));			\
		(vec)->_p = &((vec)->_v[(vec)->_n]);			\
	}								\
}									\
static inline void							\
name##_add(name *vec, type elem)					\
{									\
	while ((vec)->_n + 1 > (vec)->_n_alloced) {			\
		(vec)->_n_alloced *= 2;					\
		(vec)->_v = my_realloc((vec)->_v, (vec)->_n_alloced	\
				   * sizeof(type));			\
		(vec)->_p = &((vec)->_v[(vec)->_n]);			\
	}								\
	(vec)->_v[(vec)->_n] = elem;					\
	(vec)->_n += 1;							\
	(vec)->_p = &((vec)->_v[(vec)->_n]);				\
}									\
static inline void							\
name##_append(name *vec, type const *elems, size_t n_elems)		\
{									\
	name##_reserve(vec, n_elems);					\
	memcpy((vec)->_v + (vec)->_n, elems, (n_elems) * sizeof(type));	\
	(vec)->_n += (n_elems);						\
	(vec)->_p = &((vec)->_v[(vec)->_n]);				\
}									\
static inline void							\
name##_extend(name *vec0, name *vec1)					\
{									\
	name##_append(vec0, (vec1)->_v, (vec1)->_n);			\
}									\
static inline void							\
name##_reset(name *vec)							\
{									\
	(vec)->_n = 0;							\
	if ((vec)->_n_alloced > (vec)->_hint) {				\
		(vec)->_n_alloced = (vec)->_hint;			\
		(vec)->_v = my_realloc((vec)->_v, (vec)->_n_alloced	\
				   * sizeof(type));			\
	}								\
	(vec)->_p = &(vec->_v[0]);					\
}									\
static inline void							\
name##_clip(name *vec, size_t n_elems)					\
{									\
	if (n_elems < (vec)->_n) {					\
		(vec)->_n = n_elems;					\
		(vec)->_p = &((vec)->_v[(vec)->_n]);			\
	}								\
}									\
static inline size_t							\
name##_bytes(name *vec)							\
{									\
	return ((vec)->_n * sizeof(type));				\
}									\
static inline size_t							\
name##_size(name *vec)							\
{									\
	return ((vec)->_n);						\
}									\
static inline type							\
name##_value(name *vec, size_t i)					\
{									\
	assert(i < (vec)->_n);						\
	return ((vec)->_v[i]);						\
}									\
static inline type *							\
name##_ptr(name *vec)							\
{									\
	return ((vec)->_p);						\
}									\
static inline type *							\
name##_data(name *vec)							\
{									\
	return ((vec)->_v);						\
}									\
static inline void							\
name##_advance(name *vec, size_t x)					\
{									\
	assert(x <= ((vec)->_n_alloced - (vec)->_n));			\
	(vec)->_n += x;							\
	(vec)->_p = &((vec)->_v[(vec)->_n]);				\
}
