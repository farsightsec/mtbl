/*
 * Copyright (c) 2013 by Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>

#include "my_alloc.h"

#include "my_queue.h"

#if defined(__GNUC__)
# define _aligned __attribute__((aligned(64)))
#else
# define _aligned
#endif

struct my_queue {
	void		**elems;
	unsigned	size;
	unsigned	head;
	unsigned	tail;
	pthread_mutex_t	lock _aligned;
};

struct my_queue *
my_queue_init(unsigned size)
{
	struct my_queue *q;
	if (size < 2 || ((size - 1) & size) != 0)
		return (NULL);
	q = my_calloc(1, sizeof(*q));
	q->size = size;
	q->elems = my_calloc(size, sizeof(void *));
	pthread_mutex_init(&q->lock, NULL);
	return (q);
}

void
my_queue_destroy(struct my_queue **q)
{
	if (*q) {
		pthread_mutex_destroy(&(*q)->lock);
		free((*q)->elems);
		free(*q);
		*q = NULL;
	}
}

const char *
my_queue_impl_type(void)
{
	return ("pthread mutex");
}

static inline void
q_lock(struct my_queue *q)
{
	int rc = pthread_mutex_lock(&q->lock);
	assert(rc == 0);
}

static inline void
q_unlock(struct my_queue *q)
{
	int rc = pthread_mutex_unlock(&q->lock);
	assert(rc == 0);
}

static inline unsigned
q_space(unsigned head, unsigned tail, unsigned size)
{
	return ((tail - (head + 1)) & (size - 1));
}

static inline unsigned
q_count(unsigned head, unsigned tail, unsigned size)
{
	return ((head - tail) & (size - 1));
}

bool
my_queue_insert(struct my_queue *q, void *item, unsigned *pspace)
{
	q_lock(q);
	bool res = false;
	unsigned head = q->head;
	unsigned tail = q->tail;
	unsigned space = q_space(head, tail, q->size);
	if (space >= 1) {
		q->elems[head] = item;
		q->head = (head + 1) & (q->size - 1);
		res = true;
		space--;
	}
	q_unlock(q);
	if (pspace)
		*pspace = space;
	return (res);
}

bool
my_queue_remove(struct my_queue *q, void **pitem, unsigned *pcount)
{
	q_lock(q);
	bool res = false;
	unsigned head = q->head;
	unsigned tail = q->tail;
	unsigned count = q_count(head, tail, q->size);
	if (count >= 1) {
		*pitem = q->elems[tail];
		q->tail = (tail + 1) & (q->size - 1);
		res = true;
		count--;
	}
	q_unlock(q);
	if (pcount)
		*pcount = count;
	return (res);
}
