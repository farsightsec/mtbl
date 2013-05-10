#ifndef MY_ZONEFILE_H
#define MY_ZONEFILE_H

#include <ldns/ldns.h>

struct zonefile;

struct zonefile *
zonefile_init_fname(const char *fname);

void
zonefile_destroy(struct zonefile **);

const ldns_rdf *
zonefile_get_domain(struct zonefile *);

size_t
zonefile_get_count(struct zonefile *);

uint32_t
zonefile_get_serial(struct zonefile *);

ldns_status
zonefile_read(struct zonefile *, ldns_rr **);

#endif /* MY_ZONEFILE_H */
