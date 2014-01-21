#include <pthread.h>

#include "threadnum.h"

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int next_threadnum = 0;
static __thread int threadnum;

void
my_threadnum_register(void) {
	pthread_mutex_lock(&lock);
	threadnum = next_threadnum;
	next_threadnum += 1;
	pthread_mutex_unlock(&lock);
}

int
my_threadnum(void) {
	return (threadnum);
}
