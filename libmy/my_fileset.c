/*
 * Copyright (c) 2012 by Farsight Security, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "my_alloc.h"
#include "my_fileset.h"
#include "ubuf.h"
#include "vector.h"

struct fileset_entry {
	bool			keep;
	char			*fname;
	void			*ptr;
};

VECTOR_GENERATE(entry_vec, struct fileset_entry *);

struct my_fileset {
	ino_t			last_ino;
	time_t			last_mtime;
	char			*setfile;
	char			*setdir;
	my_fileset_load_func	load;
	my_fileset_unload_func	unload;
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
setfile_updated(struct my_fileset *fs)
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
	struct fileset_entry *fs0 = *((struct fileset_entry **) a);
	struct fileset_entry *fs1 = *((struct fileset_entry **) b);
	assert(a != NULL);
	assert(b != NULL);
	assert(fs0 != NULL);
	assert(fs1 != NULL);
	assert(fs0->fname != NULL);
	assert(fs1->fname != NULL);
	return (strcmp(fs0->fname, fs1->fname));
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

struct my_fileset *
my_fileset_init(
	const char *setfile,
	my_fileset_load_func load,
	my_fileset_unload_func unload,
	void *user)
{
	assert(path_exists(setfile));
	struct my_fileset *fs = my_calloc(1, sizeof(*fs));
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
my_fileset_destroy(struct my_fileset **fs)
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
my_fileset_user(struct my_fileset *fs)
{
	return (fs->user);
}

void
my_fileset_reload(struct my_fileset *fs)
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

	fp = fopen(fs->setfile, "r");
	if (fp == NULL)
		return;

	u = ubuf_init(64);
	new_entries = entry_vec_init(1);

	while (getline(&line, &len, fp) != -1) {
		ubuf_clip(u, 0);
		ubuf_add_cstr(u, fs->setdir);
		ubuf_add(u, '/');
		ubuf_add_cstr(u, line);
		ubuf_rstrip(u, '\n');
		fname = ubuf_cstr(u);
		if (path_exists(fname)) {
			entptr = fetch_entry(fs->entries, fname);
			if (entptr == NULL) {
				ent = my_calloc(1, sizeof(*ent));
				ent->fname = my_strdup(fname);
				if (fs->load)
					ent->ptr = fs->load(fs, fname);
				entry_vec_add(new_entries, ent);
			} else {
				ent = my_calloc(1, sizeof(*ent));
				ent->fname = my_strdup(fname);
				ent->ptr = (*entptr)->ptr;
				(*entptr)->keep = true;
				entry_vec_add(new_entries, ent);
			}
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
		assert(ent != NULL);
		if (ent->keep == false && fs->unload)
			fs->unload(fs, ent->fname, ent->ptr);
		free(ent->fname);
		free(ent);
	}
	entry_vec_destroy(&fs->entries);
	fs->entries = new_entries;
	ubuf_destroy(&u);
}

bool
my_fileset_get(
	struct my_fileset *fs,
	size_t i,
	const char **fname_out,
	void **ptr_out)
{
	if (i < entry_vec_size(fs->entries)) {
		*fname_out = entry_vec_value(fs->entries, i)->fname;
		*ptr_out = entry_vec_value(fs->entries, i)->ptr;
		return (true);
	}
	return (false);
}
