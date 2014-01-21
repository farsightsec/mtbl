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
	int		dir_fd;
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

	s->dir_fd = dirfd(s->dir);
	assert(s->dir_fd != -1);

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
	struct stat sb;
	struct dirent *de;
	char *ret = NULL;
	size_t retsz;
	char *fname = NULL;
	ubuf *src_fname;

	pthread_mutex_lock(&s->lock);

	while (fname == NULL) {
		while ((de = readdir(s->dir)) != NULL) {
			if (de->d_name[0] == '.')
				continue;
			if (fstatat(s->dir_fd, de->d_name, &sb, 0) == -1) {
				fprintf(stderr, "%s: fstatat() failed: %s\n",
					__func__, strerror(errno));
				continue;
			}
			if (!S_ISREG(sb.st_mode))
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
