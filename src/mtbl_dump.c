#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <mtbl.h>
#include "print_string.h"

static void
dump(const char *fname)
{
	const uint8_t *key, *val;
	size_t len_key, len_val;
	struct mtbl_reader *r;
	struct mtbl_iter *it;

	r = mtbl_reader_init(fname, NULL);
	assert(r != NULL);

	it = mtbl_reader_iter(r);
	assert(it != NULL);

	while (mtbl_iter_next(it, &key, &len_key, &val, &len_val)) {
		print_string(key, len_key, stdout);
		fputc(' ', stdout);
		print_string(val, len_val, stdout);
		fputc('\n', stdout);
	}
	
	mtbl_iter_destroy(&it);
	mtbl_reader_destroy(&r);
}

int
main(int argc, char **argv)
{
	char *fname;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <MTBL FILE>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	fname = argv[1];

	dump(fname);

	return (EXIT_SUCCESS);
}
