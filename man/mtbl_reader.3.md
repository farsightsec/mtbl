% MTBL_READER(3)
%
%

# NAME

mtbl_reader - read an MTBL file

# SYNOPSIS

**#include \<mtbl.h\>**

Reader objects:

**struct mtbl_reader \***
:   **mtbl_reader_init(const char \*_fname_,
                       const struct mtbl_reader_options \*_ropt_);**

**struct mtbl_reader \***
:   **mtbl_reader_init_fd(int _fd_,
                          const struct mtbl_reader_options \*_ropt_);**

**void**
:   **mtbl_reader_destroy(struct mtbl_reader \*\*_r_);**

**struct mtbl_iter \***
:   **mtbl_reader_iter(struct mtbl_reader \*_r_);**

**mtbl_res**
:   **mtbl_reader_get(struct mtbl_reader \*_r_,
                      const uint8_t \*_key_, size_t _len_key_,
                      uint8_t \*\*_val_, size_t \*_len_val_);**

**struct mtbl_iter \***
:   **mtbl_reader_get_range(struct mtbl_reader \*_r_,
                            const uint8_t \*_key0_, size_t _len_key0_,
                            const uint8_t \*_key1_, size_t _len_key1_);**

**struct mtbl_iter \***
:   **mtbl_reader_get_prefix(struct mtbl_reader \*_r_,
                             const uint8_t \*_key_, size_t _len_key_);**


Reader options:

**struct mtbl_reader_options \***
:   **mtbl_reader_options_init(void);**

**void**
:   **mtbl_reader_options_destroy(struct mtbl_reader_options \*\*_ropt_);**

**void**
:   **mtbl_reader_options_set_verify_checksums(struct mtbl_reader_options \*_ropt_,
                                               bool _verify_);**

# DESCRIPTION

MTBL files are accessed by creating an **mtbl_reader** object, calling
**mtbl_reader_get**(), **mtbl_reader_get_range**(), or
**mtbl_reader_get_prefix**() to retrieve individual values, and then finalizing
the **mtbl_reader** object by calling **mtbl_reader_destroy**().

**mtbl_reader** objects may be created by calling **mtbl_reader_init**() with an
_fname_ argument specifying the filename to be opened.  Or,
**mtbl_reader_init_fd**() may be called with an _fd_ argument specifying an open
file descriptor. Since MTBL files are immutable, the same MTBL file may be
opened and read from concurrently.

The **mtbl_reader_get**() function retrieves the value associated with a
particular key. The key is specified by the _key_ and _len_key_ arguments, and a
copy of the value and its length will be returned in the _val_ and _len_val_
arguments. This copy will be allocated with the system memory allocator and must
be freed by the caller with **free**().

The **mtbl_reader_get_range**() and **mtbl_reader_get_prefix**() functions allow
the retrieval of multiple key-value entries through the **mtbl_iter** interface.
**mtbl_reader_get_range**() retrieves all entries whose keys are between _key0_
and _key1_ inclusive, while **mtbl_reader_get_prefix**() retrieves all entries
whose keys start with the byte value specified in _key_.

**mtbl_reader_get**() is not thread safe. **mtbl_reader_get_range**() and
**mtbl_reader_get_prefix**() may be called on **mtbl_reader** objects without
locking, but the **mtbl_reader** object must not be destroyed for the duration
of the **mtbl_iter** objects thus obtained. **mtbl_iter** objects are also not
thread safe. Thus, to safely use an **mtbl_reader** object from multiple threads
without locking, each thread should acquire and use its own iterators
exclusively.

If the _ropt_ parameter to **mtbl_reader_init**() or **mtbl_reader_init_fd**()
is non-NULL, the parameters specified in the **mtbl_reader_options** object will
be configured into the **mtbl_reader** object.

## Reader options

`verify_checksums`
:   Specifies whether or not the CRC32C checksum on each data block should be
    verified or not. If _verify_checksums_ is enabled, a checksum mismatch will
    cause a runtime error. Note that the checksum on the index block is always
    verified, since the overhead of doing this once when the reader object is
    instantiated is minimal. The default is to not verify data block checksums.

# RETURN VALUE

**mtbl_reader_init**(), **mtbl_reader_init_fd**(), and
**mtbl_reader_options_init**() return NULL on failure, and
non-NULL on success.

**mtbl_reader_get**() returns **mtbl_res_success** if _key_ exists in the table
and _val_ was successfully retrieved, and **mtbl_res_failure** otherwise.

**mtbl_reader_get_range**() and **mtbl_reader_get_prefix**() return a non-NULL
**mtbl_iter** object if matching entries are available, and NULL otherwise.

# EXAMPLE

~~~
#include <mtbl.h>

const uint8_t *key, *val;
size_t len_key, len_val;
struct mtbl_reader *r;
struct mtbl_iter *it;

r = mtbl_reader_init("example.mtbl", NULL);
it = mtbl_reader_iter(r);

while (mtbl_iter_next(it, &key, &len_key, &val, &len_val) == mtbl_res_success) {
    /* process entry */
}

mtbl_iter_destroy(&it);
mtbl_reader_destroy(&r);
~~~
