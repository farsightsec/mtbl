#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <mtbl.h>

#include "block_builder.c"

#define NAME	"test-block_builder"

#include <ctype.h>

static void
print_string(const uint8_t *data, size_t len, FILE *out) {
        unsigned c;

        fputc('\'', out);
        while (len-- != 0) {
                c = *(data++);
                if (isprint(c))
                        fputc(c, out);
                else
                        fprintf(out, "\\x%02x", c);
        }
        fputc('\'', out);
        fputc('\n', out);
}

static int
test1(void)
{
	int ret = 0;
	struct block_builder *b;
	uint8_t *buf;
	size_t bufsz;

	b = block_builder_init();
	assert(b != NULL);

	fprintf(stderr, "block_builder_current_size_estimate(): %zd\n",
		block_builder_current_size_estimate(b));

	block_builder_add(b, (uint8_t *) "foobar1", 7, (uint8_t *) "42", 2);
	block_builder_add(b, (uint8_t *) "foobar2", 7, (uint8_t *) "43", 2);
	block_builder_add(b, (uint8_t *) "foobar2ZZ", 9, (uint8_t *) "44", 2);
	block_builder_add(b, (uint8_t *) "foobar3", 7, (uint8_t *) "45", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "46", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "47", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "48", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "49", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "50", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "51", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "52", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "53", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "54", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "55", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "56", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "57", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "58", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "59", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "60", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "61", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "62", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "63", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "64", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "ZA", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "ZB", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "ZC", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "ZD", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "ZE", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "ZF", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "ZG", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "ZH", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "ZI", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "ZJ", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "ZK", 2);
	block_builder_add(b, (uint8_t *) "foobar1Y", 8, (uint8_t *) "ZM", 2);

	fprintf(stderr, "block_builder_current_size_estimate(): %zd\n",
		block_builder_current_size_estimate(b));

	block_builder_finish(b, &buf, &bufsz);
	fprintf(stderr, "buf= %p bufsz= %zd\n", buf, bufsz);

	print_string(buf, bufsz, stderr);

	block_builder_reset(b);
	block_builder_destroy(&b);

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

	ret |= check(test1(), "test1");

	if (ret)
		return (EXIT_FAILURE);
	return (EXIT_SUCCESS);
}
