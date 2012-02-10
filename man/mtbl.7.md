% MTBL(7)
%
% 

# NAME

mtbl - immutable sorted string library

# SYNOPSIS

**#include \<mtbl.h\>**

**gcc** [*flags*] *files* **-lmtbl** [*libraries*]

# DESCRIPTION

The mtbl library provides interfaces for creating, searching, and merging
Sorted String Table (SSTable) files iin the _MTBL_ format, which provide an
immutable mapping of keys to values. Keys and values are arbitrary byte
arrays, and SSTables may not contain duplicate keys.

The five main interfaces provided by the mtbl library are:

**mtbl_iter**(3)

:   Iterator objects provide a consistent interface for iterating over the
    key-value entries returned by the **mtbl_reader**, **mtbl_sorter**, and
    **mtbl_merger** interfaces.

**mtbl_writer**(3)

:   Writer objects initialize a new _MTBL_ file from a sequence of key-value
    entries provided by the caller. Keys must be in sorted order based on
    lexicographical byte value, and keys may not be duplicated.

**mtbl_reader**(3)

:   Reader objects allow _MTBL_ files written by the **mtbl_writer**
    interface to be searched. An exact key match may be retrieved directly,
    while searches that can return multiple entries use the **mtbl_iter**
    interface.

**mtbl_sorter**(3)

:   Sorter objects receive a sequence of key-value entries provided by the
    caller and return them in sorted order. The caller must provide a
    callback function to merge values in the case of entries with duplicate
    keys. The sorted output sequence may be retrieved via the **mtbl_iter**
    interface or by dumping the contents to an **mtbl_writer** object.

**mtbl_merger**(3)

:   Merger objects receive multiple sequences of key-value entries from one
    or more **mtbl_reader** objects and combine them into a single, sorted
    sequence. The merged output sequence may be retrieved via the
    **mtbl_iter** interface or by dumping the contents to an **mtbl_writer**
    object.

Additionally, several utility interfaces are provided:

**mtbl_crc32c**(3)

:   Calculates the CRC32C checksum of a byte array.

**mtbl_fixed**(3)

:   Functions for fixed-width encoding and decoding of 32 and 64 bit integers.

**mtbl_varint**(3)

:   Functions for varint encoding and decoding of 32 and 64 bit integers.
