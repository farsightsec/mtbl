#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

/* 
 * This is the pre-compiled function which is used to generate test-merge.so,
 * which is the DSO fed to mtbl_merge during the test-merge.sh unit test.
 */
void
test_merge_func(void *clos,
	const uint8_t *key, size_t len_key,
	const uint8_t *val0, size_t len_val0,
	const uint8_t *val1, size_t len_val1,
	uint8_t **merged_val, size_t *len_merged_val)
{
	assert(len_val0 > 0);
	assert(len_val1 > 0);
	*len_merged_val = len_val0 + len_val1;
	
	*merged_val = calloc(1, *len_merged_val);
	memcpy(*merged_val, val0, len_val0);
	memcpy(*merged_val + len_val0, val1, len_val1);	
}
