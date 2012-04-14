/*
 * Copyright (c) 2012 by Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fileset.h"
#include "my_alloc.h"
#include "ubuf.h"
#include "vector.h"

struct fileset_entry {
	char			*fname;
	void			*ptr;
};

VECTOR_GENERATE(entry_vec, struct fileset_entry *);

struct fileset {
	ino_t			last_ino;
	time_t			last_mtime;
	char			*setfile;
	char			*setdir;
	fileset_load_func	load;
	fileset_unload_func	unload;
	void			*user;
	entry_vec		*entries;
};

static bool
path_exists(const char *path)
{
	struct stat sb;
	int ret;

	ret = stat(path, &sb);
	if (ret < 0)
		return (false);
	return (true);
}

static bool
setfile_updated(struct fileset *fs)
{
	struct stat ss;
	int ret;

	ret = stat(fs->setfile, &ss);
	if (ret < 0) {
		fprintf(stderr, "%s: stat('%s') failed: %s\n",
			__func__, fs->setfile, strerror(errno));
		return (false);
	}
	if (fs->last_ino != ss.st_ino || fs->last_mtime != ss.st_mtime) {
		fs->last_ino = ss.st_ino;
		fs->last_mtime = ss.st_mtime;
		return (true);
	}
	return (false);
}

static int
cmp_fileset_entry(const void *a, const void *b)
{
	assert(a != NULL);
	assert(b != NULL);
	struct fileset_entry *ent0 = *((struct fileset_entry **) a);
	struct fileset_entry *ent1 = *((struct fileset_entry **) b);
	if (ent0 == NULL)
		return (1);
	if (ent1 == NULL)
		return (-1);
	assert(ent0->fname != NULL);
	assert(ent1->fname != NULL);
	return (strcmp(ent0->fname, ent1->fname));
}

static struct fileset_entry **
fetch_entry(entry_vec *entries, char *fname)
{
	struct fileset_entry **ent;
	struct fileset_entry key = { .fname = fname, .ptr = NULL };
	struct fileset_entry *pkey = &key;
	ent = bsearch(&pkey,
		      entry_vec_data(entries),
		      entry_vec_size(entries),
		      sizeof(void *),
		      cmp_fileset_entry);
	return (ent);
}

struct fileset *
fileset_init(
	const char *setfile,
	fileset_load_func load,
	fileset_unload_func unload,
	void *user)
{
	assert(path_exists(setfile));
	struct fileset *fs = my_calloc(1, sizeof(*fs));
	char *t = my_strdup(setfile);
	fs->setdir = my_strdup(dirname(t));
	free(t);
	fs->setfile = my_strdup(setfile);
	fs->load = load;
	fs->unload = unload;
	fs->user = user;
	fs->entries = entry_vec_init(1);
	return (fs);
}

void
fileset_destroy(struct fileset **fs)
{
	if (*fs != NULL) {
		for (size_t i = 0; i < entry_vec_size((*fs)->entries); i++) {
			struct fileset_entry *ent = entry_vec_value((*fs)->entries, i);
			if ((*fs)->unload)
				(*fs)->unload(*fs, ent->fname, ent->ptr);
			free(ent->fname);
			free(ent);
		}
		entry_vec_destroy(&(*fs)->entries);
		free((*fs)->setdir);
		free((*fs)->setfile);
		free(*fs);
		*fs = NULL;
	}
}

void *
fileset_user(struct fileset *fs)
{
	return (fs->user);
}

void
fileset_reload(struct fileset *fs)
{
	assert(fs != NULL);
	struct fileset_entry *ent, **entptr;
	entry_vec *new_entries;
	FILE *fp;
	char *fname, *line = NULL;
	size_t len = 0;
	ubuf *u;

	if (!setfile_updated(fs))
		return;

	u = ubuf_init(64);
	new_entries = entry_vec_init(1);

	fp = fopen(fs->setfile, "r");
	if (fp == NULL) {
		fprintf(stderr, "%s: fopen('%s') failed: %s",
			__func__, fs->setfile, strerror(errno));
		return;
	}

	while (getline(&line, &len, fp) != -1) {
		ubuf_clip(u, 0);
		ubuf_add_cstr(u, fs->setdir);
		ubuf_add(u, '/');
		ubuf_add_cstr(u, line);
		ubuf_rstrip(u, '\n');
		fname = ubuf_cstr(u);
		if (path_exists(fname)) {
			entptr = fetch_entry(new_entries, fname);
			if (entptr != NULL) {
				fprintf(stderr, "%s: warning: duplicate filename '%s' in "
					"fileset index\n", __func__, fname);
				continue;
			}
			entptr = fetch_entry(fs->entries, fname);
			if (entptr == NULL) {
				ent = my_calloc(1, sizeof(*ent));
				ent->fname = my_strdup(fname);
				if (fs->load)
					ent->ptr = fs->load(fs, fname);
				entry_vec_add(new_entries, ent);
			} else {
				fprintf(stderr, "%s: moving %s into new_entries vector\n",
					__func__, fname);
				ent = *entptr;
				*entptr = NULL;
				entry_vec_add(new_entries, ent);
			}
		} else {
			fprintf(stderr, "%s: file vanished: %s\n", __func__, ubuf_cstr(u));
		}
	}
	free(line);
	fclose(fp);

	qsort(entry_vec_data(new_entries),
	      entry_vec_size(new_entries),
	      sizeof(void *),
	      cmp_fileset_entry);

	for (size_t i = 0; i < entry_vec_size(fs->entries); i++) {
		ent = entry_vec_value(fs->entries, i);
		if (ent != NULL) {
			if (fs->unload)
				fs->unload(fs, ent->fname, ent->ptr);
			free(ent->fname);
			free(ent);
		}
	}
	entry_vec_destroy(&fs->entries);
	fs->entries = new_entries;
	ubuf_destroy(&u);
}

bool
fileset_get(struct fileset *fs, size_t i, const char **fname_out, void **ptr_out)
{
	if (i < entry_vec_size(fs->entries)) {
		*fname_out = entry_vec_value(fs->entries, i)->fname;
		*ptr_out = entry_vec_value(fs->entries, i)->ptr;
		return (true);
	}
	return (false);
}
