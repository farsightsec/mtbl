#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <ldns/ldns.h>

#include "my_alloc.h"
#include "ubuf.h"
#include "zonefile.h"

struct zonefile {
	FILE		*fp;
	bool		eof;
	bool		is_pipe;
	bool		valid;
	ldns_rdf	*domain;
	ldns_rdf	*origin;
	ldns_rdf	*prev;
	ldns_rr		*rr_soa;
	uint32_t	ttl;
	size_t		count;
};

static ldns_status
read_soa(struct zonefile *z)
{
	ldns_rr *rr;
	ldns_status status;

	for (;;) {
		status = ldns_rr_new_frm_fp_l(&rr, z->fp, &z->ttl, &z->origin, &z->prev, NULL);
		switch (status) {
		case LDNS_STATUS_OK:
			goto out;
		case LDNS_STATUS_SYNTAX_EMPTY:
		case LDNS_STATUS_SYNTAX_TTL:
		case LDNS_STATUS_SYNTAX_ORIGIN:
			status = LDNS_STATUS_OK;
			break;
		default:
			goto out;
		}
	}
out:
	if (status != LDNS_STATUS_OK) {
		z->valid = false;
		return (LDNS_STATUS_ERR);
	}

	if (ldns_rr_get_type(rr) != LDNS_RR_TYPE_SOA) {
		ldns_rr_free(rr);
		z->valid = false;
		return (LDNS_STATUS_ERR);
	}

	z->count = 1;
	z->domain = ldns_rdf_clone(ldns_rr_owner(rr));
	z->origin = ldns_rdf_clone(ldns_rr_owner(rr));
	z->rr_soa = rr;
	return (LDNS_STATUS_OK);
}

struct zonefile *
zonefile_init_fname(const char *fname)
{
	struct zonefile *z = my_calloc(1, sizeof(struct zonefile));

	size_t len_fname = strlen(fname);
	if (len_fname >= 3 &&
	    fname[len_fname - 3] == '.' &&
	    fname[len_fname - 2] == 'g' &&
	    fname[len_fname - 1] == 'z')
	{
		ubuf *u = ubuf_new();
		ubuf_add_cstr(u, "zcat ");
		ubuf_add_cstr(u, fname);
		z->fp = popen(ubuf_cstr(u), "r");
		z->is_pipe = true;
		ubuf_destroy(&u);
	} else {
		z->fp = fopen(fname, "r");
	}

	if (z->fp == NULL)
		return (NULL);

	z->valid = true;
	if (read_soa(z) != LDNS_STATUS_OK)
		zonefile_destroy(&z);

	return (z);
}

void
zonefile_destroy(struct zonefile **z)
{
	if (*z) {
		if ((*z)->fp) {
			if ((*z)->is_pipe)
				pclose((*z)->fp);
			else
				fclose((*z)->fp);
		}
		if ((*z)->origin)
			ldns_rdf_deep_free((*z)->origin);
		if ((*z)->prev)
			ldns_rdf_deep_free((*z)->prev);
		if ((*z)->domain)
			ldns_rdf_deep_free((*z)->domain);
		if ((*z)->rr_soa)
			ldns_rr_free((*z)->rr_soa);
		free(*z);
		*z = NULL;
	}
}

const ldns_rdf *
zonefile_get_domain(struct zonefile *z)
{
	return (z->domain);
}

size_t
zonefile_get_count(struct zonefile *z)
{
	return (z->count);
}

uint32_t
zonefile_get_serial(struct zonefile *z)
{
	ldns_rdf *rdf = ldns_rr_rdf(z->rr_soa, 2);
	assert(rdf != NULL);
	return (ldns_rdf2native_int32(rdf));
}

ldns_status
zonefile_read(struct zonefile *z, ldns_rr **out)
{
	ldns_rr *rr;
	ldns_status status = LDNS_STATUS_OK;

	if (z->eof) {
		*out = NULL;
		return (LDNS_STATUS_OK);
	}

	if (!z->valid)
		return (LDNS_STATUS_ERR);

	if (z->count == 1 && z->rr_soa != NULL) {
		*out = z->rr_soa;
		z->rr_soa = NULL;
		return (LDNS_STATUS_OK);
	}
	for (;;) {
		if (feof(z->fp)) {
			*out = NULL;
			z->eof = true;
			return (LDNS_STATUS_OK);
		}
		status = ldns_rr_new_frm_fp_l(&rr, z->fp, &z->ttl, &z->origin, &z->prev, NULL);
		switch (status) {
		case LDNS_STATUS_OK:
			if (ldns_rr_get_type(rr) == LDNS_RR_TYPE_SOA) {
				ldns_rr_free(rr);
				*out = NULL;
				return (LDNS_STATUS_OK);
			}
			z->count++;
			goto out;
		case LDNS_STATUS_SYNTAX_EMPTY:
		case LDNS_STATUS_SYNTAX_TTL:
		case LDNS_STATUS_SYNTAX_ORIGIN:
			status = LDNS_STATUS_OK;
			break;
		default:
			goto out;
		}
	}
out:
	if (status != LDNS_STATUS_OK)
		return (status);
	*out = rr;
	return (status);
}
