#ifndef RSF_ALLOC
#define RSF_ALLOC

#include <assert.h>
#include <stdlib.h>

static inline void *
my_calloc(size_t nmemb, size_t size)
{
	void *ptr = calloc(nmemb, size);
	assert(ptr != NULL);
	return (ptr);
}

static inline void *
my_malloc(size_t size)
{
	void *ptr = malloc(size);
	assert(ptr != NULL);
	return (ptr);
}

static inline void *
my_realloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	assert(ptr != NULL);
	return (ptr);
}

#endif /* RSF_ALLOC */
