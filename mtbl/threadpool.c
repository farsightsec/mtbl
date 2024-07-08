/*
 * Copyright (c) 2024 DomainTools LLC
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

#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <stdbool.h>

#include "threadpool.h"

struct thread {
	pthread_mutex_t m;
	pthread_cond_t c;
	pthread_t t;

	void *arg, *res;

	thread_cb cb;

	struct thread *next;
	struct resultq *rq;
	struct threadpool *pool;

	bool running;
};

struct resultq {
	pthread_mutex_t m;
	pthread_cond_t c;

	bool finished;
	size_t nthreads;

	struct thread *head, **ptail;
};

struct threadpool {
	pthread_mutex_t m;
	pthread_cond_t c;
	struct thread *head;

	size_t count;
	size_t max;
};

struct result_handler {
	pthread_t thread;
	struct resultq *rq;
	result_cb cb;
	void *cbdata;
};

/* Threadpool */

static void *
thread_worker(void *arg)
{
	struct thread *me = arg;

	for (;;) {
		struct resultq *rq;

		pthread_mutex_lock(&me->m);
		while (!me->running)
			pthread_cond_wait(&me->c, &me->m);
		pthread_mutex_unlock(&me->m);

		if (me->cb == NULL)
			break;

		me->res = me->cb(me->arg);
		me->cb = NULL;
		me->arg = NULL;

		rq = me->rq;
		me->rq = NULL;

		if (rq != NULL) {
			/*
			 * We were dispatched with ordered = false; place self
			 * onto given queue when finished. Since the thread enters
			 * the queue in a finished state (running = false), there
			 * is no need to signal the thread condition.
			 */
			me->running = false;

			pthread_mutex_lock(&rq->m);
			*rq->ptail = me;
			rq->ptail = &me->next;
			pthread_cond_signal(&rq->c);
			pthread_mutex_unlock(&rq->m);
		} else {
			pthread_mutex_lock(&me->m);
			me->running = false;
			pthread_cond_signal(&me->c);
			pthread_mutex_unlock(&me->m);
		}
	}

	return NULL;
}

struct threadpool *
threadpool_init(size_t max_threads)
{
	struct threadpool *pool = calloc(1, sizeof(*pool));

	pthread_mutex_init(&pool->m, NULL);
	pthread_cond_init(&pool->c, NULL);
	pool->max = max_threads;

	return pool;
}

static struct thread *
threadpool_next(struct threadpool *pool)
{
	struct thread *thr = NULL;

	pthread_mutex_lock(&pool->m);

	while (pool->head == NULL && pool->count == pool->max) {
		pthread_cond_wait(&pool->c, &pool->m);
	}

	if (pool->head != NULL) {
		thr = pool->head;
		pool->head = thr->next;
		thr->next = NULL;
		assert(thr->cb == NULL);
		assert(thr->res == NULL);
		assert(!thr->running);
	} else {
		pool->count++;
	}

	pthread_mutex_unlock(&pool->m);

	if (thr == NULL) {
		thr = calloc(1, sizeof(*thr));
		thr->pool = pool;
		pthread_mutex_init(&thr->m, NULL);
		pthread_cond_init(&thr->c, NULL);
		pthread_create(&thr->t, NULL, thread_worker, thr);
	}

	return thr;
}

void
threadpool_dispatch(struct threadpool *pool,
		    struct result_handler *rh,
		    bool ordered,
		    thread_cb cb,
		    void *arg)
{
	struct resultq *rq = rh->rq;
	struct thread *thr = threadpool_next(pool);

	assert(!thr->running);
	assert(thr->next == NULL);

	pthread_mutex_lock(&thr->m);
	thr->rq = ordered?NULL:rq;
	thr->cb = cb;
	thr->arg = arg;
	thr->running = true;
	pthread_cond_signal(&thr->c);
	pthread_mutex_unlock(&thr->m);

	pthread_mutex_lock(&rq->m);
	assert(!rq->finished);
	rq->nthreads++;
	if (ordered) {
		*rq->ptail = thr;
		rq->ptail = &thr->next;
		pthread_cond_signal(&rq->c);
	}
	pthread_mutex_unlock(&rq->m);
}

void
threadpool_destroy(struct threadpool **poolp)
{
	struct threadpool *pool = *poolp;
	struct thread *thr;

	if (pool == NULL)
		return;

	pthread_mutex_lock(&pool->m);
	while (pool->count > 0) {
		while (pool->head == NULL)
			pthread_cond_wait(&pool->c, &pool->m);

		thr = pool->head;
		pool->head = thr->next;

		assert(thr->cb == NULL);

		/* Wake up thread with NULL callback, signaling exit */
		pthread_mutex_lock(&thr->m);
		thr->running = true;
		pthread_cond_signal(&thr->c);
		pthread_mutex_unlock(&thr->m);

		pthread_join(thr->t, NULL);
		pthread_cond_destroy(&thr->c);
		pthread_mutex_destroy(&thr->m);
		free(thr);

		pool->count--;
	}
	pthread_mutex_unlock(&pool->m);

	pthread_mutex_destroy(&pool->m);
	pthread_cond_destroy(&pool->c);
	free(pool);

	*poolp = NULL;
}


/* Resultq */

static struct resultq *
resultq_init(void)
{
	struct resultq *rq = calloc(1, sizeof(*rq));

	pthread_mutex_init(&rq->m, NULL);
	pthread_cond_init(&rq->c, NULL);

	rq->ptail = &rq->head;
	return rq;
}

static bool
resultq_next(struct resultq *rq, void **res)
{
	struct thread *thr = NULL;

	/* Remove thread from queue, if any */
	pthread_mutex_lock(&rq->m);
	while (rq->head == NULL && !(rq->finished && rq->nthreads == 0))
		pthread_cond_wait(&rq->c, &rq->m);

	if (rq->head != NULL) {
		thr = rq->head;
		rq->head = thr->next;
		thr->next = NULL;
		rq->nthreads--;

		if (rq->head == NULL)
			rq->ptail = &rq->head;
	}
	pthread_mutex_unlock(&rq->m);

	if (thr == NULL)
		return false;

	/* Wait for thread's result */
	pthread_mutex_lock(&thr->m);
	while(thr->running)
		pthread_cond_wait(&thr->c, &thr->m);
	*res = thr->res;
	thr->res = NULL;
	pthread_mutex_unlock(&thr->m);

	/* Return thread to pool */
	pthread_mutex_lock(&thr->pool->m);
	thr->next = thr->pool->head;
	thr->pool->head = thr;
	pthread_cond_signal(&thr->pool->c);
	pthread_mutex_unlock(&thr->pool->m);

	return true;
}

static void
resultq_finish(struct resultq *rq)
{
	pthread_mutex_lock(&rq->m);
	rq->finished = true;
	pthread_cond_signal(&rq->c);
	pthread_mutex_unlock(&rq->m);
}

static void
resultq_destroy(struct resultq **rqp)
{
	struct resultq *rq = *rqp;

	if (rq == NULL)
		return;

	assert(rq->head == NULL && rq->finished && rq->nthreads == 0);
	pthread_mutex_destroy(&rq->m);
	pthread_cond_destroy(&rq->c);
	free(rq);
	*rqp = NULL;
}


/* Result Handler */

static void *
result_worker(void *arg)
{
	struct result_handler *rh = arg;
	void *res;

	while (resultq_next(rh->rq, &res))
		rh->cb(res, rh->cbdata);

	resultq_destroy(&rh->rq);

	return NULL;
}

struct result_handler *
result_handler_init(result_cb cb, void *cbdata)
{
	struct result_handler *rh = calloc(1, sizeof(*rh));

	rh->rq = resultq_init();
	rh->cb = cb;
	rh->cbdata = cbdata;
	pthread_create(&rh->thread, NULL, result_worker, rh);

	return rh;
}

void
result_handler_destroy(struct result_handler **prh)
{
	struct result_handler *rh = *prh;
	if (rh == NULL) return;
	resultq_finish(rh->rq);
	pthread_join(rh->thread, NULL);
	free(rh);
	*prh = NULL;
}
