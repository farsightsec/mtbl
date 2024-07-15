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

/* Public interface */

struct mtbl_threadpool {
	struct threadpool *pool;
};

/* External API, through mtbl wrapper */
struct threadpool;

/*
 * Initialize a pool of at most `max_threads` threads.
 * This pool can be passed to multiple thread users.
 */
struct threadpool *threadpool_init(size_t max_threads);

/*
 * Destroy a pool of threads, after waiting for all
 * threads to finish.
 */
void threadpool_destroy(struct threadpool **poolp);


/* Internal API. */
struct result_handler;

typedef void *(*thread_cb)(void *arg);
typedef void (*result_cb)(void *res, void *cbdata);

/* Initialize a handler for worker thread results. */
struct result_handler *result_handler_init(result_cb cb, void *cbdata);

/*
 * threadpool_dispatch runs `cb(arg)` in a thread from pool `pool`,
 * passing its result to the result handler's callback.
 *
 * If `ordered` is true, results will be passed to the result handler
 * callback in the order `threadpool_dispatch` was called, otherwise
 * they will be passed as each worker thread finishes.
 */

void threadpool_dispatch(struct threadpool *pool,
			 struct result_handler *rh,
			 bool ordered,
			 thread_cb cb, void *arg);

/*
 * Free all resources associated with the result handler *rhp after
 * waiting for the result handler thread and all worker threads to
 * finish.
 */
void result_handler_destroy(struct result_handler **rhp);
