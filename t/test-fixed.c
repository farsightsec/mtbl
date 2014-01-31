#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <mtbl.h>

#define NAME	"test-fixed"

static int
do_64(uint64_t val)
{
	uint8_t buf[8];

	mtbl_fixed_encode64(buf, val);
	fprintf(stderr, "%s: buf= 0x%02x%02x%02x%02x%02x%02x%02x%02x val= %" PRIu64 "\n",
		__func__,
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
		val);
	if (mtbl_fixed_decode64(buf) != val) {
		fprintf(stderr, "%s: val= %" PRIu64 " failed\n", __func__, val);
		return (0);
	}

	return (1);
}

static int
test2(void)
{
	int ret = 0;
	unsigned count = 0;
	uint64_t val;

	if (!do_64(0) || !do_64(1) || !do_64(0xffffffffffffffffU))
		ret |= 1;

	for (uint64_t i = 1; i < UINT64_MAX; i *= 17) {
		val = i++;
		if (!do_64(val))
			ret |= 1;
		if (count++ > 10)
			break;
	}

	return (ret);
}

static int
do_32(uint32_t val)
{
	uint8_t buf[4];

	mtbl_fixed_encode32(buf, val);
	fprintf(stderr, "%s: buf= 0x%02x%02x%02x%02x val= %u\n",
		__func__, buf[0], buf[1], buf[2], buf[3], val);
	if (mtbl_fixed_decode32(buf) != val) {
		fprintf(stderr, "%s: val= %u failed\n", __func__, val);
		return (0);
	}

	return (1);
}

static int
test1(void)
{
	int ret = 0;

	if (!do_32(0) || !do_32(1) || !do_32(0xffffffffU))
		ret |= 1;

	for (uint64_t i = 1; i < UINT32_MAX; i *= 23) {
		uint32_t val = (uint32_t) i++;
		if (!do_32(val))
			ret |= 1;
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
