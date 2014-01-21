#ifndef MY_BYTEORDER_H
#define MY_BYTEORDER_H

#include "config.h"

#ifdef HAVE_ENDIAN_H
# include <endian.h>
#else
# ifdef HAVE_SYS_ENDIAN_H
#  include <sys/endian.h>
# endif
#endif

#if HAVE_DECL_HTOLE32
# define my_htole32 htole32
#else
# if defined(WORDS_BIGENDIAN)
#  define my_htole32 my_bswap32
# else
#  define my_htole32(x) (x)
# endif
#endif

#if HAVE_DECL_LE32TOH
# define my_le32toh le32toh
#else
# if defined(WORDS_BIGENDIAN)
#  define my_le32toh my_bswap32
# else
#  define my_le32toh(x) (x)
# endif
#endif

static inline uint32_t
my_bswap32(uint32_t x)
{
	return  ((x << 24) & 0xff000000 ) |
		((x <<  8) & 0x00ff0000 ) |
		((x >>  8) & 0x0000ff00 ) |
		((x >> 24) & 0x000000ff );
}

#endif /* MY_BYTEORDER_H */
