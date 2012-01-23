#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <mtbl.h>

#define NAME	"test-varint"

static int
test2(void)
{
	int ret = 0;
	uint8_t buf[8192];
	uint8_t *end;
	uint8_t *p = buf;

	for (uint32_t i = 0; i < 64; i++) {
		const uint64_t power = 1ull << i;

		mtbl_varint_encode64(&p, power);
		mtbl_varint_encode64(&p, power - 1);
		mtbl_varint_encode64(&p, power + 1);
	}

	end = p;
	p = buf;
	for (uint32_t i = 0; i < 64; i++) {
		const uint64_t power = 1ull << i;
		uint64_t actual, expected;

		actual = mtbl_varint_decode64(&p);
		expected = power;
		if (actual != expected) {
			ret |= 1;
			fprintf(stderr, "expected= %" PRIu64 ", actual= %" PRIu64 ", res= %u\n",
				expected, actual, actual == expected);
		}

		actual = mtbl_varint_decode64(&p);
		expected = power - 1;
		if (actual != expected) {
			ret |= 1;
			fprintf(stderr, "expected= %" PRIu64 ", actual= %" PRIu64 ", res= %u\n",
				expected, actual, actual == expected);
		}

		actual = mtbl_varint_decode64(&p);
		expected = power + 1;
		if (actual != expected) {
			ret |= 1;
			fprintf(stderr, "expected= %" PRIu64 ", actual= %" PRIu64 ", res= %u\n",
				expected, actual, actual == expected);
		}
	}

	if (p != end) {
		ret |= 1;
		fprintf(stderr, "p = %p, end = %p\n", p, end);
	}

	return (ret);
}

static int
test1(void)
{
	int ret = 0;
	uint8_t buf[8192];
	uint8_t *end;
	uint8_t *p = buf;

	for (uint32_t i = 0; i < (32 * 32); i++) {
		uint32_t v = (i / 32) << (i % 32);
		mtbl_varint_encode32(&p, v);
	}

	end = p;
	p = buf;
	for (uint32_t i = 0; i < (32 * 32); i++) {
		uint32_t expected = (i / 32) << (i % 32);
		uint32_t actual = mtbl_varint_decode32(&p);
		if (expected != actual) {
			ret |= 1;
			fprintf(stderr, "expected= %u, actual= %u\n", expected, actual);
		}
	}

	if (p != end) {
		ret |= 1;
		fprintf(stderr, "p = %p, end = %p\n", p, end);
	}

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
	ret |= check(test2(), "test2");

	if (ret)
		return (EXIT_FAILURE);
	return (EXIT_SUCCESS);
}
