% MTBL_MERGER(3)
%
%

# NAME

mtbl_merger - merge multiple MTBL input files into a single output

# SYNOPSIS

**#include \<mtbl.h\>**

Merger objects:

**struct mtbl_merger \***
:   **mtbl_merger_init(const struct mtbl_merger_options \*_mopt_);**

**void**
:   **mtbl_merger_destroy(struct mtbl_merger \*\*_m_);**

**mtbl_res**
:   **mtbl_merger_add_reader(struct mtbl_merger \*_m_, struct mtbl_reader \*_r_);**

**mtbl_res**
:   **mtbl_merger_write(struct mtbl_merger \*_m_, struct mtbl_writer \*_w_);**

**struct mtbl_iter \***
:   **mtbl_merger_iter(struct mtbl_merger \*_m_);**

Merger options:

**struct mtbl_merger_options \***
:   **mtbl_merger_options_init(void);**

**void**
:   **mtbl_merger_options_destroy(struct mtbl_merger_options \*\*_mopt_);**

**void**
:   **mtbl_merger_options_set_merge_func(struct mtbl_merger_options \*_mopt_, mtbl_merge_func _fp_, void \*_clos_);**

**typedef void(\*mtbl_merge_func)(void \*_clos_, const uint8_t \*_key_, size_t _len_key_, const uint8_t \*_val0_, size_t _len_val0_, const uint8_t \*_val1_, size_t _len_val1_, uint8_t \*\*_merged_val_, size_t \*_len_merged_val_);**

# DESCRIPTION

Multiple MTBL input files may be merged together using the **mtbl_merger**
interface, which reads key-value entries from one or more MTBL files and
provides these entries in sorted order. The sorted entries may be consumed via
the **mtbl_iter** interface, or they may be dumped to an **mtbl_writer** object.
Because the MTBL format does not allow duplicate keys, the caller must provide a
function which will accept a key and two conflicting values for that key and
return a replacement value. This function may be called multiple times for the
same key if more than two inputs are being merged.

**mtbl_merger** objects are created with the **mtbl_merger_init**() function,
which requires a non-NULL _mopt_ argument which has been configured with the
merge function _fp_.

One or more **mtbl_reader** objects must be provided as input to the
**mtbl_merger** object by calling **mtbl_merger_add_reader**(). After the
desired inputs have been configured, either **mtbl_merger_write**() or
**mtbl_merger_iter**() should be called in order to consume the merged output.
It is a runtime error to call **mtbl_merger_add_reader**() on an **mtbl_merger**
object after iteration has begun, and once the merged output has been consumed,
it is also a runtime error to call any other function but
**mtbl_merger_destroy**() on the depleted **mtbl_merger** object.

## Merger options

`merge_func`
:   This option specifies the merge function callback, consisting of a function
    pointer _fp_ and a pointer to user data _clos_ which will be passed as the
    first argument to _fp_. The remaining arguments to the merge function are:

    _key_ -- pointer to the key for which there exist duplicate values.

    _len_key_ -- length of the key.

    _val0_ -- pointer to the first value.

    _len_val_0_ -- length of the first value.

    _val1_ -- pointer to the second value.

    _len_val_1_ -- length of the second value.

    _merged_val_ -- pointer to where the callee should place its merged value.

    _len_merged_val_ -- pointer to where the callee should place the length of
    its merged value.

    The callee may provide an empty value as the merged value, in which case
    _merged_val_ must still contain a non-NULL value and _len_merged_val_ must
    contain the value 0.

    _merged_val_ must be allocated with the system allocator, and the
    **mtbl_merger** takes responsibility for free()ing the value once it is no
    longer needed.


# RETURN VALUE

If the merge function callback is unable to provide a merged value (that is, it
fails to return a non-NULL value in its _merged_val_ argument, or it explicitly
returns the value NULL), the merge process will be aborted, and
**mtbl_merger_write**() or **mtbl_iter_next**() will return
**mtbl_res_failure**.

**mtbl_merger_add_reader**() returns **mtbl_res_success** if the **mtbl_reader**
object was successfully added, and **mtbl_res_failure** otherwise.

**mtbl_merger_write**() returns **mtbl_res_success** if the merged output was
successfully written, and **mtbl_res_failure** otherwise.
