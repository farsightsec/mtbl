#ifndef MY_FILESET_H
#define MY_FILESET_H

#include <stdbool.h>

struct my_fileset;

typedef void *(*my_fileset_load_func)(struct my_fileset *, const char *fname);
typedef void (*my_fileset_unload_func)(struct my_fileset *, const char *fname, void *);

struct my_fileset *my_fileset_init(
	const char *setfile,
	my_fileset_load_func,
	my_fileset_unload_func,
	void *user);
void my_fileset_destroy(struct my_fileset **);
void *my_fileset_user(struct my_fileset *);
void my_fileset_reload(struct my_fileset *);
bool my_fileset_get(struct my_fileset *, size_t, const char **, void **);

#endif /* MY_FILESET_H */
