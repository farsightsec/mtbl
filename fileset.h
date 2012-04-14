#ifndef RSF_FILESET_H
#define RSF_FILESET_H

#include <stdbool.h>

struct fileset;

typedef void *(*fileset_load_func)(struct fileset *, const char *fname);
typedef void (*fileset_unload_func)(struct fileset *, const char *fname, void *);

struct fileset *fileset_init(
	const char *setfile,
	fileset_load_func,
	fileset_unload_func,
	void *user);
void fileset_destroy(struct fileset **);
void *fileset_user(struct fileset *);
void fileset_reload(struct fileset *);
bool fileset_get(struct fileset *, size_t, const char **, void **);

#endif /* RSF_FILESET_H */
