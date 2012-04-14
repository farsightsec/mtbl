#ifndef RSF_FILESET_H
#define RSF_FILESET_H

#include <stdbool.h>

struct rsf_fileset;

typedef void *(*rsf_fileset_load_func)(struct rsf_fileset *, const char *fname);
typedef void (*rsf_fileset_unload_func)(struct rsf_fileset *, const char *fname, void *);

struct rsf_fileset *rsf_fileset_init(
	const char *setfile,
	rsf_fileset_load_func,
	rsf_fileset_unload_func,
	void *user);
void rsf_fileset_destroy(struct rsf_fileset **);
void *rsf_fileset_user(struct rsf_fileset *);
void rsf_fileset_reload(struct rsf_fileset *);
bool rsf_fileset_get(struct rsf_fileset *, size_t, const char **, void **);

#endif /* RSF_FILESET_H */
