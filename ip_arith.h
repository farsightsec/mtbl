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

#ifndef RSF_IPARITH_H
#define RSF_IPARITH_H

#ifdef HAVE_ENDIAN_H
# include <endian.h>
#else
# ifdef HAVE_SYS_ENDIAN_H
#  include <sys/endian.h>
# endif
#endif

#include <stdint.h>
#include <string.h>

/* given big endian IPv4 prefix in 'address'/'prefix_len',
 * write lowest IP address ("network address") into 'out' */
static inline void
ip4_lower(const void *address, unsigned prefix_len, void *out) {
	uint32_t addr;
	memcpy(&addr, address, 4);
	addr &= htobe32(~((1 << (32 - prefix_len)) - 1));
	memcpy(out, &addr, 4);
}

/* given big endian IPv4 prefix in 'address'/'prefix_len',
 * write highest IP address ("broadcast address") into 'out' */
static inline void
ip4_upper(const void *address, unsigned prefix_len, void *out) {
	uint32_t addr;
	memcpy(&addr, address, 4);
	addr |= htobe32((1 << (32 - prefix_len)) - 1);
	memcpy(out, &addr, 4);
}

/* increment IPv4 address by 1 */
static inline void
ip4_incr(void *address) {
	uint32_t addr;
	memcpy(&addr, address, 4);
	addr = be32toh(addr) + 1;
	addr = htobe32(addr);
	memcpy(address, &addr, 4);
}

/* given big endian IPv6 prefix in 'address'/'prefix_len',
 * write lowest IP address ("network address") into 'lower' */
static inline void
ip6_lower(const void *address, unsigned prefix_len, void *lower)
{
	uint64_t addr[2], mask[2];
	if (prefix_len < 64) {
		mask[0] = htobe64(((1ULL << prefix_len) - 1ULL) << (64ULL - prefix_len));
		mask[1] = htobe64(0);
	} else if (prefix_len < 128) {
		prefix_len -= 64;
		mask[0] = htobe64(UINT64_MAX);
		mask[1] = htobe64(((1ULL << prefix_len) - 1ULL) << (64ULL - prefix_len));
	} else {
		memset(mask, 0xFF, sizeof(mask));
	}
	memcpy(addr, address, 16);
	addr[0] &= mask[0];
	addr[1] &= mask[1];
	memcpy(lower, addr, 16);
}

/* given big endian IPv6 prefix in 'address'/'prefix_len',
 * write highest IP address ("broadcast address") into 'upper' */
static inline void
ip6_upper(const void *address, unsigned prefix_len, void *upper)
{
	uint64_t addr[2], mask[2];
	if (prefix_len < 64) {
		mask[0] = htobe64(~(((1ULL << prefix_len) - 1ULL) << (64ULL - prefix_len)));
		mask[1] = htobe64(UINT64_MAX);
	} else if (prefix_len < 128) {
		prefix_len -= 64;
		mask[0] = htobe64(0);
		mask[1] = htobe64(~(((1ULL << prefix_len) - 1ULL) << (64ULL - prefix_len)));
	} else {
		memset(&mask, 0, sizeof(mask));
	}
	memcpy(addr, address, 16);
	addr[0] |= mask[0];
	addr[1] |= mask[1];
	memcpy(upper, addr, 16);
}

/* increment IPv6 address by 1 */
static inline void
ip6_incr(void *address) {
	uint64_t addr[2];
	memcpy(addr, address, 16);
	addr[0] = be64toh(addr[0]);
	addr[1] = be64toh(addr[1]);
	if (addr[1] == UINT64_MAX) {
		addr[0] += 1;
		addr[1] = 0;
	} else {
		addr[1] += 1;
	}
	addr[0] = htobe64(addr[0]);
	addr[1] = htobe64(addr[1]);
	memcpy(address, addr, 16);
}

#endif /* RSF_IPARITH_H */
