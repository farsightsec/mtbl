#ifndef MY_GETENV_INT_H
#define MY_GETENV_INT_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

static inline bool
getenv_int(const char *name, uint64_t *value) {
	char *s, *t;

	s = getenv(name);
	if (s == NULL)
		return (false);

	*value = strtoul(s, &t, 0);
	if (*t != '\0')
		return (false);
	return (true);
}

#endif /* MY_GETENV_INT_H */
