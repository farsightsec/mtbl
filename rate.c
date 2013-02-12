/*
 * Copyright (c) 2008, 2009, 2012, 2013 by Internet Systems Consortium, Inc. ("ISC")
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

#include <stdint.h>
#include <math.h>

#include "my_alloc.h"
#include "my_time.h"

#include "rate.h"

struct rate {
	double		start;
	uint64_t	count;
	unsigned	rate, freq;
};

struct rate *
rate_init(unsigned rate, unsigned freq)
{
	struct rate *r;
	struct timespec ts;

	r = my_calloc(1, sizeof(*r));
	my_gettime(CLOCK_MONOTONIC, &ts);
	r->start = ts.tv_sec + ts.tv_nsec / 1E9;
	r->rate = rate;
	r->freq = freq;
	return (r);
}

void
rate_destroy(struct rate **r)
{
	if (*r) {
		free(*r);
		*r = NULL;
	}
}

void
rate_sleep(struct rate *r)
{
	r->count += 1;
	if (r->count % r->freq == 0) {
		struct timespec ts;
		double d, cur_rate, over;

		my_gettime(CLOCK_MONOTONIC, &ts);
		d = (ts.tv_sec + ts.tv_nsec / 1E9) - r->start;
		cur_rate = r->count / d;
		over = cur_rate - r->rate;

		if (over > 0.0) {
			double my_sleep;

			my_sleep = over / cur_rate;
			ts.tv_sec = floor(my_sleep);
			ts.tv_nsec = (my_sleep - ts.tv_sec) * 1E9;
			my_nanosleep(&ts);
		}
	}
}
