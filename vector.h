/*
 * Copyright (c) 2012-2014 by Farsight Security, Inc.
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

#include <assert.h>

#include "my_alloc.h"

#define VECTOR_GENERATE(name, type)					\
typedef struct name##__vector {						\
	type *		_v;						\
	type *		_p;						\
	size_t		_n, _n_alloced, _hint;				\
} name;									\
__attribute__((unused))							\
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
__attribute__((unused))							\
static inline void							\
name##_reinit(unsigned hint, name *vec)					\
{									\
	if (hint == 0) hint = 1;					\
	vec->_hint = vec->_n_alloced = hint;				\
	vec->_n = 0;							\
	vec->_v = my_malloc(vec->_n_alloced * sizeof(type));		\
	vec->_p = &(vec->_v[0]);					\
}									\
__attribute__((unused))							\
static inline void							\
name##_detach(name *vec, type **out, size_t *outsz)			\
{									\
	*(out) = (vec)->_v;						\
	*(outsz) = (vec)->_n;						\
	(vec)->_n = 0;							\
	(vec)->_n_alloced = (vec)->_hint;				\
	(vec)->_v = my_malloc((vec)->_n_alloced * sizeof(type));	\
	(vec)->_p = &(vec->_v[0]);					\
}									\
__attribute__((unused))							\
static inline void							\
name##_destroy(name **vec)						\
{									\
	if (*vec) {							\
		my_free((*vec)->_v);					\
		my_free((*vec));					\
	}								\
}									\
__attribute__((unused))							\
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
__attribute__((unused))							\
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
__attribute__((unused))							\
static inline void							\
name##_append(name *vec, type const *elems, size_t n_elems)		\
{									\
	name##_reserve(vec, n_elems);					\
	memcpy((vec)->_v + (vec)->_n, elems, (n_elems) * sizeof(type));	\
	(vec)->_n += (n_elems);						\
	(vec)->_p = &((vec)->_v[(vec)->_n]);				\
}									\
__attribute__((unused))							\
static inline void							\
name##_extend(name *vec0, name *vec1)					\
{									\
	name##_append(vec0, (vec1)->_v, (vec1)->_n);			\
}									\
__attribute__((unused))							\
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
__attribute__((unused))							\
static inline void							\
name##_clip(name *vec, size_t n_elems)					\
{									\
	if (n_elems < (vec)->_n) {					\
		(vec)->_n = n_elems;					\
		(vec)->_p = &((vec)->_v[(vec)->_n]);			\
	}								\
}									\
__attribute__((unused))							\
static inline size_t							\
name##_bytes(name *vec)							\
{									\
	return ((vec)->_n * sizeof(type));				\
}									\
__attribute__((unused))							\
static inline size_t							\
name##_size(name *vec)							\
{									\
	return ((vec)->_n);						\
}									\
__attribute__((unused))							\
static inline type							\
name##_value(name *vec, size_t i)					\
{									\
	assert(i < (vec)->_n);						\
	return ((vec)->_v[i]);						\
}									\
__attribute__((unused))							\
static inline type *							\
name##_ptr(name *vec)							\
{									\
	return ((vec)->_p);						\
}									\
__attribute__((unused))							\
static inline type *							\
name##_data(name *vec)							\
{									\
	return ((vec)->_v);						\
}									\
__attribute__((unused))							\
static inline void							\
name##_advance(name *vec, size_t x)					\
{									\
	assert(x <= ((vec)->_n_alloced - (vec)->_n));			\
	(vec)->_n += x;							\
	(vec)->_p = &((vec)->_v[(vec)->_n]);				\
}
