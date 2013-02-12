/*
 * Copyright (c) 2008, 2013 by Internet Systems Consortium, Inc. ("ISC")
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

#ifndef RSF_RATE_H
#define RSF_RATE_H

/*
 * Tight loops can be slowed down by repeated calls to rate_sleep(). This
 * works best when the amount of time elapsed between successive calls is
 * approximately the same.
 */

struct rate;

/**
 * Initialize a new struct rate object.
 *
 * 'freq' should usually be about 10% of 'rate'.
 *
 * \param[in] rate target rate limit in Hz, greater than 0.
 *
 * \param[in] freq how often the rate limit will be checked, i.e., every 'freq'
 *	calls to nmsg_rate_sleep(), greater than 0.
 */
struct rate *
rate_init(unsigned rate, unsigned freq);

void
rate_destroy(struct rate **r);

void
rate_sleep(struct rate *r);

#endif /* RSF_RATE_H */
