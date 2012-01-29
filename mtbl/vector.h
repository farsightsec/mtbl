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

#define VECTOR_GENERATE(name, type)				\
typedef struct name##__vector {					\
	type *		v;					\
	type *		p;					\
	unsigned	n, n_alloced, hint;			\
} name;								\
static inline name *						\
name##_init(unsigned hint)					\
{								\
	name *vec;						\
	vec = calloc(1, sizeof(name));				\
	assert(vec != NULL);					\
	if (hint == 0) hint = 1;				\
	vec->hint = vec->n_alloced = hint;			\
	vec->v = malloc(vec->n_alloced * sizeof(void *));	\
	assert(vec->v != NULL);					\
	vec->p = &(vec->v[0]);					\
	return (vec);						\
}								\
static inline void						\
name##_destroy(name **vec)					\
{								\
	free((*vec)->v);					\
	free((*vec));						\
	*vec = NULL;						\
}								\
static inline void						\
name##_add(name *vec, type elem)				\
{								\
	if ((vec)->n == (vec)->n_alloced - 1) {			\
		(vec)->n_alloced *= 2;				\
		(vec)->v = realloc((vec)->v, (vec)->n_alloced	\
				   * sizeof(void *));		\
		assert((vec)->v != NULL);			\
	}							\
	(vec)->v[(vec)->n] = elem;				\
	(vec)->n += 1;						\
	(vec)->p = &((vec)->v[(vec)->n]);			\
}								\
static inline void						\
name##_need(name *vec, size_t n)				\
{								\
	while ((n) > ((vec)->n_alloced - (vec)->n)) {		\
		(vec)->n_alloced *= 2;				\
		(vec)->v = realloc((vec)->v, (vec)->n_alloced	\
				   * sizeof(void *));		\
		assert((vec)->v != NULL);			\
	}							\
}								\
static inline void						\
name##_append(name *vec, const type *elems, size_t n)		\
{								\
	name##_need(vec, n);					\
	memcpy((vec)->v + (vec)->n, elems, (n)*sizeof(*elems));	\
	(vec)->n += (n) / sizeof(type);				\
	(vec)->p = &((vec)->v[(vec)->n]);			\
}								\
static inline void						\
name##_reset(name *vec)						\
{								\
	(vec)->n = 0;						\
	if ((vec)->n_alloced > (vec)->hint) {			\
		(vec)->n_alloced = (vec)->hint;			\
		(vec)->v = realloc((vec)->v, (vec)->n_alloced	\
				   * sizeof(void *));		\
		assert((vec)->v != NULL);			\
	}							\
	(vec)->p = &(vec->v[0]);				\
}								\
static inline size_t						\
name##_bytes(name *vec)						\
{								\
	return ((vec)->n * sizeof(type));			\
}								\
static inline size_t						\
name##_size(name *vec)						\
{								\
	return ((vec)->n);					\
}								\
static inline type						\
name##_value(name *vec, size_t i)				\
{								\
	assert(i < (vec)->n);					\
	return ((vec)->v[i]);					\
}								\
static inline type *						\
name##_ptr(name *vec)						\
{								\
	return ((vec)->p);					\
}								\
static inline void						\
name##_advance(name *vec, size_t x)				\
{								\
	assert(x < ((vec)->n_alloced - (vec)->n));		\
	(vec)->n += x;						\
	(vec)->p = &((vec)->v[(vec)->n]);			\
}
