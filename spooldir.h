#ifndef MY_SPOOLDIR_H
#define MY_SPOOLDIR_H

struct spooldir;

struct spooldir *spooldir_init(const char *path);
void spooldir_destroy(struct spooldir **);
char *spooldir_next(struct spooldir *);

#endif /* MY_SPOOLDIR_H */
