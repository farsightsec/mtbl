% MTBL_CRC32C(3)
%
%

# NAME

mtbl_crc32c - calculate CRC32C checksum

# SYNOPSIS

**#include \<mtbl.h\>**

**uint32_t**
:   **mtbl_crc32c(const uint8_t \*_buffer_, size_t _length_);**

# DESCRIPTION

The **mtbl_crc32c**() function calculates the CRC32C checksum of a sequence of
bytes of length _length_. The _buffer_ argument points to the start of the
sequence.

# RETURN VALUE

The CRC32C checksum.
