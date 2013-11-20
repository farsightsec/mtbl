/*
 * Copyright (c) 2012 by Farsight Security, Inc.
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

#ifndef MY_IP_ARITH_H
#define MY_IP_ARITH_H

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

#endif /* MY_IP_ARITH_H */
