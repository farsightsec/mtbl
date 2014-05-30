/*
 * Copyright (c) 2008, 2009, 2012, 2013, 2014 by Farsight Security, Inc.
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

#include <stdint.h>
#include <math.h>

#include "my_alloc.h"
#include "my_time.h"

#include "my_rate.h"

struct my_rate {
	struct timespec next_tick, period, start;
	uint64_t	count;
	unsigned	adj_rate, rate, freq;
};

static inline int64_t
ts_nanos(struct timespec *ts)
{
	return (ts->tv_sec * 1000000000 + ts->tv_nsec);
}

static inline struct timespec
calc_next_tick(const struct timespec *t, const struct timespec *m)
{
	struct timespec res;

	res = *t;
	if (m->tv_sec > 0) {
		res.tv_sec -= (res.tv_sec % m->tv_sec);
		res.tv_sec += m->tv_sec;
	}
	if (m->tv_nsec > 0) {
		res.tv_nsec -= (res.tv_nsec % m->tv_nsec);
		res.tv_nsec += m->tv_nsec;
	} else {
		res.tv_nsec = 0;
	}

	while (res.tv_nsec >= 1000000000) {
		res.tv_sec += 1;
		res.tv_nsec -= 1000000000;
	}

	return (res);
}

static inline void
adjust_rate(struct my_rate *r, struct timespec *now)
{
	struct timespec elapsed;
	double ratio;
	unsigned actual_rate;

	/* amount of time elapsed since first sleep */
	elapsed = *now;
	my_timespec_sub(&r->start, &elapsed);

	/* the average event rate that has been maintained over the
	 * lifespan of this rate-limiter.
	 */
	actual_rate = r->count / (ts_nanos(&elapsed) / (1000000000 + 0.0));

	/* simple ratio of nominal event rate and average event rate */
	ratio = r->rate / (actual_rate + 0.0);

	/* clamp this ratio to a small interval */
	if (ratio < 0.99)
		ratio = 0.99;
	if (ratio > 1.01)
		ratio = 1.01;

	/* calculate a new, adjusted rate based on this ratio */
	r->adj_rate *= ratio;

	/* calculate a new tick period based on the adjusted rate */
	const double period = 1.0 / (r->adj_rate + 0.0);
	my_timespec_from_double(period, &r->period);
}

struct my_rate *
my_rate_init(unsigned rate, unsigned freq)
{
	struct my_rate *r;

	r = calloc(1, sizeof(*r));
	if (r == NULL)
		return (NULL);
	r->adj_rate = rate;
	r->rate = rate;
	r->freq = freq;

	/* calculate the tick period */
	const double period = 1.0 / (r->rate + 0.0);
	my_timespec_from_double(period, &r->period);

	return (r);
}

void
my_rate_destroy(struct my_rate **r)
{
	if (*r != NULL) {
		free(*r);
		*r = NULL;
	}
}

void
my_rate_sleep(struct my_rate *r)
{
	struct timespec now, til;

	/* what clock to use depends on whether clock_nanosleep() is available */
#if HAVE_CLOCK_GETTIME
# if HAVE_CLOCK_NANOSLEEP
	static const clockid_t rate_clock = CLOCK_MONOTONIC;
# else
	static const clockid_t rate_clock = CLOCK_REALTIME;
# endif
#else
	static const int rate_clock = -1;
#endif

	if (r == NULL)
		return;

	/* update the event counter */
	r->count += 1;

	/* fetch the current time */
	my_gettime(rate_clock, &now);

	/* special case: if this is the first call to rate_sleep(),
	 * calculate when the next tick will be. this is a little bit more
	 * accurate than calculating it in rate_init().
	 */
	if (r->count == 1) {
		r->start = now;
		r->next_tick = calc_next_tick(&now, &r->period);
	}

	/* adjust the rate and period every 'freq' events.
	 * skip the first window of 'freq' events.
	 * disabled if 'freq' is 0.
	 */
	if (r->freq != 0 && (r->count % r->freq) == 0 && r->count > r->freq)
		adjust_rate(r, &now);

	/* 'til', amount of time remaining until the next tick */
	til = r->next_tick;
	my_timespec_sub(&now, &til);

	/* if 'til' is in the past, don't bother sleeping */
	if (ts_nanos(&til) > 0) {
		/* do the sleep */
#if HAVE_CLOCK_NANOSLEEP
		clock_nanosleep(rate_clock, TIMER_ABSTIME, &r->next_tick, NULL);
#else
		struct timespec rel;
		rel = r->next_tick;
		my_timespec_sub(&now, &rel);
		my_nanosleep(&rel);
#endif

		/* re-fetch the current time */
		my_gettime(rate_clock, &now);
	}

	/* calculate the next tick */
	r->next_tick = calc_next_tick(&now, &r->period);
}
