% MTBL_WRITER(3)
%
%

# NAME

mtbl_writer - create an MTBL file

# SYNOPSIS

**#include \<mtbl.h\>**

Writer objects:

**struct mtbl_writer \***
:   **mtbl_writer_init(const char \*_fname_,
                       const struct mtbl_writer_options \*_wopt_);**

**struct mtbl_writer \***
:   **mtbl_writer_init_fd(int _fd_,
                          const struct mtbl_writer_options \*_wopt_);**

**void**
:   **mtbl_writer_destroy(struct mtbl_writer \*\*_w_);**

**mtbl_res**
:   **mtbl_writer_add(struct mtbl_writer \*_w_,
                      const uint8_t \*_key_, size_t _len_key_,
                      const uint8_t \*_val_, size_t _len_val_);**

Writer options:

**struct mtbl_writer_options \***
:   **mtbl_writer_options_init(void);**

**void**
:   **mtbl_writer_options_destroy(struct mtbl_writer_options \*\*_wopt_);**

**void**
:   **mtbl_writer_options_set_compression(struct mtbl_writer_options \*_wopt_,
                                          mtbl_compression_type _compression_);**

**void**
:   **mtbl_writer_options_set_block_size(struct mtbl_writer_options \*_wopt_,
                                         size_t _block_size_);**

**void**
:   **mtbl_writer_options_set_block_restart_interval(struct mtbl_writer_options \*_wopt_,
                                                     size_t _block_restart_interval_);**

# DESCRIPTION

MTBL files are written to disk by creating an **mtbl_writer** object, calling
**mtbl_writer_add**() for each key-value entry, and then finalizing the
**mtbl_writer** object by calling **mtbl_writer_destroy**().

**mtbl_writer_add**() copies key-value pairs from the caller into the
**mtbl_writer** object. Keys are specified as a pointer to a buffer, _key_, and
the length of that buffer, _len_key_. Values are specified as a pointer to a
buffer, _val_, and the length of that buffer, _len_val_.

Keys must be in sorted, lexicographical byte order. The same key may not be
added to an **mtbl_writer** more than once.

**mtbl_writer** objects may be created by calling **mtbl_writer_init**() with an
_fname_ argument specifying a filename to be created. The filename must not
already exist on the filesystem. Or, **mtbl_writer_init_fd**() may be called
with an _fd_ argument specifying an open file descriptor. No data may have been
written to the file descriptor.

If the _wopt_ parameter to **mtbl_writer_init**() or **mtbl_writer_init_fd**()
is non-NULL, the parameters specified in the **mtbl_writer_options** object will
be configured into the **mtbl_writer** object.

## Writer options

`compression`
:   Specifies the compression algorithm to use on data blocks. Possible values
    are **MTBL_COMPRESSION_NONE**, **MTBL_COMPRESSION_SNAPPY**, or
    **MTBL_COMPRESSION_ZLIB** (the default).

`block_size`
:   The maximum size of uncompressed data blocks, specified in bytes. The
    default is 8 kilobytes.

`block_restart_interval`
:   How frequently to restart intra-block key prefix compression. The default is
    every 16 keys.

# RETURN VALUE

**mtbl_writer_init**(), **mtbl_writer_init_fd**(), and
**mtbl_writer_options_init**() return NULL on failure, and
non-NULL on success.

**mtbl_writer_add**() returns **mtbl_res_success** if the key-value entry was
successfully copied into the **mtbl_writer** object, and **mtbl_res_failure** if
not, for instance if there has been a key ordering violation.

# EXAMPLE

~~~
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mtbl.h>

char key[64];
char val[64];

struct mtbl_writer *w;
struct mtbl_writer_options *wopt;

memset(key, 0, sizeof(key));
memset(val, 0, sizeof(val));

wopt = mtbl_writer_options_init();
mtbl_writer_options_set_compression(wopt, MTBL_COMPRESSION_SNAPPY);

w = mtbl_writer_init("example.mtbl", wopt);
mtbl_writer_options_destroy(&wopt);

for (unsigned i = 0; i < 1048576; i++) {
    sprintf(key, "key%012u", i);
    sprintf(val, "val%012lu", random());
    mtbl_writer_add(w, key, strlen(key), val, strlen(val));
}

mtbl_writer_destroy(&w);
~~~
