#ifndef RSF_TIME
#define RSF_TIME

#include <sys/time.h>
#include <errno.h>
#include <time.h>

static inline void
my_timespec_sub(const struct timespec *a, struct timespec *b) {
	b->tv_sec -= a->tv_sec;
	b->tv_nsec -= a->tv_nsec;
	if (b->tv_nsec < 0) {
		b->tv_sec -= 1;
		b->tv_nsec += 1000000000;
	}
}

static inline double
my_timespec_to_double(const struct timespec *ts) {
	return (ts->tv_sec + ts->tv_nsec / 1E9);
}

static inline void
my_nanosleep(const struct timespec *ts)
{
	struct timespec rqt, rmt;

	for (rqt = *ts; nanosleep(&rqt, &rmt) < 0 && errno == EINTR; rqt = rmt)
		;
}

#endif /* RSF_TIME */
