#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <mtbl.h>

#define NAME	"test-memtable"

static char test_key[] =
	"0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz{|}~";
static char test_val[] =
	"VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVWXYZ";

#define s(k) (const uint8_t *) (k), (strlen(k)+1)

static int
test1(void)
{
	int ret = 0;
	struct mtbl_memtable *m;
	uint8_t *k, *k2;
	uint8_t *v;
	size_t k_len, v_len, k2_len;
	char key[64];
	char val[64];
	
	m = mtbl_memtable_init();

	/* check that size is 0 on a newly initialized memtable */
	if (mtbl_memtable_size(m) != 0)
		ret |= 1;

	/* add one element */
	mtbl_memtable_add(m, s(test_key), s(test_val));

	/* check that size is 1 after adding one element */
	if (mtbl_memtable_size(m) != 1)
		ret |= 1;

	/* check that we get the same element back */
	mtbl_memtable_get(m, 0, &k, &k_len, &v, &v_len);
	if (!((k_len == strlen(test_key) + 1) &&
	      (v_len == strlen(test_val) + 1) &&
	      (memcmp(test_key, k, k_len) == 0) &&
	      (memcmp(test_val, v, v_len) == 0)))
	{
		ret |= 1;
	}

	/*
	fprintf(stderr, "%s: mtbl_memtable_get(0): k= '%s' (%zd) v= '%s' (%zd)\n",
		__func__, k, k_len, v, v_len);
	*/

	for (unsigned i = 0; i < 1024; i++) {
		sprintf(key, "key%lu", random());
		sprintf(val, "val%lu", random());
		mtbl_memtable_add(m, s(key), s(val));
	}

	//fprintf(stderr, "%s: mtbl_memtable_size(): %zd\n", __func__, mtbl_memtable_size(m));

	/* finalize the memtable */
	mtbl_memtable_finish(m);

	/* verify that the memtable is in sorted order */
	mtbl_memtable_get(m, 0, &k, &k_len, NULL, NULL);
	for (unsigned i = 1; i < mtbl_memtable_size(m); i++) {
		mtbl_memtable_get(m, i, &k2, &k2_len, NULL, NULL);

		/*
		fprintf(stderr, "%s: mtbl_memtable_get(%u): key= '%s' (%zd)\n",
			__func__, i, k2, k2_len);
		*/

		if (k2_len < k_len)
			ret |= 1;

		if (k2_len == k_len) {
			if (memcmp(k, k2, k_len) >= 0)
				ret |= 1;
		}

		k = k2;
		k_len = k2_len;
	}


	/* cleanup */
	mtbl_memtable_destroy(&m);

	return (ret);
}

static int
check(int ret, const char *s)
{
	if (ret == 0)
		fprintf(stderr, NAME ": PASS: %s\n", s);
	else
		fprintf(stderr, NAME ": FAIL: %s\n", s);
	return (ret);
}

int
main(int argc, char **argv)
{
	int ret = 0;

	srandom(time(NULL));

	ret |= check(test1(), "test1");

	if (ret)
		return (EXIT_FAILURE);
	return (EXIT_SUCCESS);
}
