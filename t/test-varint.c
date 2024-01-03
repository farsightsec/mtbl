#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <mtbl.h>

#define NAME	"test-varint"


/* Test invalid varint encoding. */
static int
test4(void)
{
	uint64_t val_64;
	uint32_t val_32;
	size_t len;
	int ret = 0;

	/*
	 * A varint will continue to decode so long as the present byte has the
	 * 0x80 bit set, or until the maximum encoded length has been reached.
	 * For a 32bit varint, the max len is 5 bytes, and the 5th byte MUST
	 * be <= 0x80 in value, or decoding will fail.
	 */
	static uint8_t bad_varint_1[5] = "\xab\xc1\x91\x98\x7f";

	len = mtbl_varint_decode32(bad_varint_1, &val_32);
	if (len != 5 || val_32 != 4077150379)
		return 1;

	bad_varint_1[4]++;	/* Corrupt the last byte to be >= 0x80 */
	len = mtbl_varint_decode32(bad_varint_1, &val_32);
	if (len != 0 || val_32 != 0)
		return 1;

	/* Set the second to last byte to zero and the length decreases. */
	bad_varint_1[3] = 0;
	len = mtbl_varint_decode32(bad_varint_1, &val_32);
	if (len != 4)
		return 1;

	/*
	 * We do essentially the same thing with a 64bit varint, except here the
	 * maximum encoded length is 10 bytes rather than 5.
	 */
	static uint8_t bad_varint_2[10] = "\xff\xc1\x91\x98\x81\xff\xe3\x98\x87\x7f";

	len = mtbl_varint_decode64(bad_varint_2, &val_64);
	if (len != 10 || val_64 != 9741725764612808959UL)
		return 1;

	bad_varint_2[9]++;	/* corrupt the last byte to be >= 0x80 */
	len = mtbl_varint_decode64(bad_varint_2, &val_64);
	if (len != 0 || val_64 != 0)
		return 1;

	bad_varint_2[8] = 0;	/* truncate the decoding at 2nd to last byte */
	len = mtbl_varint_decode64(bad_varint_2, &val_64);
	if (len != 9)
		return 1;

	return ret;
}

static int
test3(void)
{
	int ret = 0;
	unsigned int lval[66];

	for (uint32_t i = 0; i < 66; i++) {
		uint64_t power = i;

		if (i == 65)
			power = ~(1);
		else if (i)
			power = 1ull << (i - 1);

		lval[i] = mtbl_varint_length(power);
	}

	for (uint32_t i = 0; i < 66; i++) {
		unsigned int expected = 0;

		if (i <= 7)
			expected = 1;
		else if ((i >= 8) && (i <= 14))
			expected = 2;
		else if ((i >= 15) && (i <= 21))
			expected = 3;
		else if ((i >= 22) && (i <= 28))
			expected = 4;
		else if ((i >= 29) && (i <= 35))
			expected = 5;
		else if ((i >= 36) && (i <= 42))
			expected = 6;
		else if ((i >= 43) && (i <= 49))
			expected = 7;
		else if ((i >= 50) && (i <= 56))
			expected = 8;
		else if ((i >= 57) && (i <= 63))
			expected = 9;
		else if (i == 64 || i == 65)
			expected = 10;

		if (expected != lval[i]) {
			ret |= 1;
			fprintf(stderr, "expected= %u actual= %u res= %u\n",
				expected, lval[i], lval[i]== expected);
		}

	}

	return (ret);
}

static int
test2(void)
{
	int ret = 0;
	uint8_t buf[8192];
	uint8_t *end;
	uint8_t *p = buf;

	for (uint32_t i = 0; i < 64; i++) {
		const uint64_t power = 1ull << i;

		p += mtbl_varint_encode64(p, power);
		p += mtbl_varint_encode64(p, power - 1);
		p += mtbl_varint_encode64(p, power + 1);
	}

	end = p;
	p = buf;
	for (uint32_t i = 0; i < 64; i++) {
		const uint64_t power = 1ull << i;
		uint64_t actual, expected;

		p += mtbl_varint_decode64(p, &actual);
		expected = power;
		if (actual != expected) {
			ret |= 1;
			fprintf(stderr, "expected= %" PRIu64 ", actual= %" PRIu64 ", res= %u\n",
				expected, actual, actual == expected);
		}

		p += mtbl_varint_decode64(p, &actual);
		expected = power - 1;
		if (actual != expected) {
			ret |= 1;
			fprintf(stderr, "expected= %" PRIu64 ", actual= %" PRIu64 ", res= %u\n",
				expected, actual, actual == expected);
		}

		p += mtbl_varint_decode64(p, &actual);
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
		p += mtbl_varint_encode32(p, v);
	}

	end = p;
	p = buf;
	for (uint32_t i = 0; i < (32 * 32); i++) {
		uint32_t expected = (i / 32) << (i % 32);
		uint32_t actual;

		p += mtbl_varint_decode32(p, &actual);
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
	ret |= check(test3(), "test3");
	ret |= check(test4(), "test3");

	if (ret)
		return (EXIT_FAILURE);
	return (EXIT_SUCCESS);
}
