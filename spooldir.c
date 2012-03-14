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
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "my_alloc.h"
#include "spooldir.h"
#include "ubuf.h"

#define UBUFSZ		128

struct spooldir {
	pthread_mutex_t	lock;
	DIR		*dir;
	ubuf		*fname;
	ubuf		*dname_active;
	ubuf		*dname_incoming;
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
path_isdir(const char *path)
{
	struct stat sb;
	int ret;

	ret = stat(path, &sb);
	if (ret < 0)
		return (false);
	if (S_ISDIR(sb.st_mode))
		return (true);
	return (false);
}

static bool
path_mkdir(const char *path, mode_t mode)
{
	int ret;

	if (path_isdir(path)) {
		return (true);
	} else {
		ret = mkdir(path, mode);
		if (ret < 0) {
			perror("mkdir");
			return (false);
		}
	}
	return (true);
}

struct spooldir *
spooldir_init(const char *path)
{
	struct spooldir *s = my_calloc(1, sizeof(*s));
	bool res;
	char *dname;

	pthread_mutex_init(&s->lock, NULL);

	dname = realpath(path, NULL);
	assert(dname != NULL);

	assert(path_isdir(dname));

	s->dname_active = ubuf_init(UBUFSZ);
	ubuf_add_cstr(s->dname_active, dname);
	ubuf_add_cstr(s->dname_active, "/active");
	res = path_mkdir(ubuf_cstr(s->dname_active), 0755);
	assert(res);

	s->dname_incoming = ubuf_init(UBUFSZ);
	ubuf_add_cstr(s->dname_incoming, dname);
	ubuf_add_cstr(s->dname_incoming, "/incoming");
	res = path_mkdir(ubuf_cstr(s->dname_incoming), 0755);
	assert(res);

	free(dname);

	s->dir = opendir(ubuf_cstr(s->dname_incoming));
	assert(s->dir != NULL);

	s->fname = ubuf_init(UBUFSZ);

	return (s);
}

void
spooldir_destroy(struct spooldir **s)
{
	if (*s != NULL) {
		pthread_mutex_destroy(&(*s)->lock);
		closedir((*s)->dir);
		ubuf_destroy(&(*s)->fname);
		ubuf_destroy(&(*s)->dname_active);
		ubuf_destroy(&(*s)->dname_incoming);
		free(*s);
		*s = NULL;
	}
}

char *
spooldir_next(struct spooldir *s)
{
	struct dirent *de;
	char *ret = NULL;
	size_t retsz;
	char *fname = NULL;
	ubuf *src_fname;

	pthread_mutex_lock(&s->lock);

	while (fname == NULL) {
		while ((de = readdir(s->dir)) != NULL) {
			assert(de->d_type != DT_UNKNOWN);
			if (de->d_type != DT_REG || de->d_name[0] == '.')
				continue;
			fname = de->d_name;
			break;
		}

		if (fname == NULL) {
			rewinddir(s->dir);
			usleep(500*1000);
			pthread_mutex_unlock(&s->lock);
			usleep(500*1000);
			return (NULL);
		}
	}

	assert(fname != NULL);

	src_fname = ubuf_init(UBUFSZ);
	ubuf_extend(src_fname, s->dname_incoming);
	ubuf_add_fmt(src_fname, "/%s", fname);
	ubuf_cterm(src_fname);

	ubuf_clip(s->fname, 0);
	ubuf_extend(s->fname, s->dname_active);
	ubuf_add_fmt(s->fname, "/%s", fname);
	ubuf_cterm(s->fname);

	if (path_exists(ubuf_cstr(s->fname))) {
		fprintf(stderr, "%s: WARNING: unlinking destination path %s\n",
			__func__, ubuf_cstr(s->fname));
		unlink(ubuf_cstr(s->fname));
	}

	int rename_ret = rename(ubuf_cstr(src_fname), ubuf_cstr(s->fname));
	if (rename_ret != 0) {
		fprintf(stderr, "rename(%s, %s): %s\n",
			ubuf_cstr(src_fname), ubuf_cstr(s->fname), strerror(errno));
		goto out;
	}

	ubuf_detach(s->fname, (uint8_t **) &ret, &retsz);
out:
	ubuf_destroy(&src_fname);
	pthread_mutex_unlock(&s->lock);
	return (ret);
}
