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

#if defined(__APPLE__)
# include <libkern/OSByteOrder.h>
# define be16toh OSSwapBigToHostInt16
# define be32toh OSSwapBigToHostInt32
# define be64toh OSSwapBigToHostInt64
# define htobe16 OSSwapHostToBigInt16
# define htobe32 OSSwapHostToBigInt32
# define htobe64 OSSwapHostToBigInt64
# define htole16 OSSwapHostToLittleInt16
# define htole32 OSSwapHostToLittleInt32
# define htole64 OSSwapHostToLittleInt64
# define le16toh OSSwapLittleToHostInt16
# define le32toh OSSwapLittleToHostInt32
# define le64toh OSSwapLittleToHostInt64
#endif

#endif /* MY_BYTEORDER_H */
