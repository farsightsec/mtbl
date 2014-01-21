#include <assert.h>
#include <stdint.h>

#include "librsf/ubuf.h"

static inline void
bytes_shortest_separator(ubuf *start, const uint8_t *limit, size_t len_limit)
{

	size_t min_length = ubuf_size(start) < len_limit ? ubuf_size(start) : len_limit;
	size_t diff_index = 0;
	while ((diff_index < min_length) &&
	       (ubuf_data(start)[diff_index] == limit[diff_index]))
	{
		diff_index++;
	}

	if (diff_index >= min_length)
		return;

	uint8_t diff_byte = ubuf_data(start)[diff_index];
	if (diff_byte < 0xFF && diff_byte + 1 < limit[diff_index]) {
		ubuf_data(start)[diff_index]++;
		ubuf_clip(start, diff_index + 1);
	} else if (diff_index < min_length - sizeof(uint16_t)) {
		/* awww yeah, big endian arithmetic on strings */
		uint16_t u_start, u_limit, u_between;
		memcpy(&u_start, &ubuf_data(start)[diff_index], sizeof(u_start));
		memcpy(&u_limit, &limit[diff_index], sizeof(u_limit));
		u_start = be16toh(u_start);
		u_limit = be16toh(u_limit);
		u_between = u_start + 1;
		if (u_start <= u_between && u_between <= u_limit) {
			u_between = htobe16(u_between);
			memcpy(&ubuf_data(start)[diff_index], &u_between, sizeof(u_between));
			ubuf_clip(start, diff_index + sizeof(uint16_t));
		}
	}

	assert(bytes_compare(ubuf_data(start), ubuf_size(start), limit, len_limit) < 0);
}
