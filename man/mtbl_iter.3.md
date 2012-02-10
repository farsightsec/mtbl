% MTBL_ITER(3)
%
%

# NAME

mtbl_iter - iterate over a sequence of key-value pairs

# SYNOPSIS

**#include \<mtbl.h\>**

**mtbl_res**
:   **mtbl_iter_next(struct mtbl_iter \*_it_,
                     const uint8_t \*\*_key_, size_t \*_len_key_,
                     const uint8_t \*\*_val_, size_t \*_len_val_);**

**void**
:   **mtbl_iter_destroy(struct mtbl_iter \*\*_it_);**

# DESCRIPTION

**mtbl_iter** objects are returned by certain **mtbl**(3) library calls which
need to provide a sequence of one or more key-value pairs. The caller should
repeatedly call **mtbl_iter_next**() until there are no more key-value entries
to retrieve, at which point the iterator object should be freed by calling
**mtbl_iter_destroy**().

# RETURN VALUE

**mtbl_iter_next**() returns **mtbl_res_success** if a key-value entry was
successfully retrieved, in which case _key_ and _val_ will point to buffers of
length _len_key_ and _len_val_ respectively. The value **mtbl_res_failure** is
returned if there are no more entries to read.

# EXAMPLE

~~~
#include <mtbl.h>

const uint8_t *key, *val;
size_t len_key, len_val;
struct mtbl_reader *r;

/* initialize r */

struct mtbl_iter *it = mtbl_reader_iter(r);

while (mtbl_iter_next(it, &key, &len_key, &val, &len_val)
        == mtbl_res_success)
{
    /* process key-value pair */
}
mtbl_iter_destroy(&it);

~~~

# SEE ALSO

**mtbl_reader(3)**, **mtbl_sorter(3)**, **mtbl_merger(3)**.
