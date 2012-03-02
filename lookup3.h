#ifndef RSF_LOOKUP3_H
#define RSF_LOOKUP3_H

#include <stdint.h>

uint32_t rsf_hashword(const uint32_t *key, size_t length, uint32_t initval);
uint32_t rsf_hashlittle(const void *key, size_t length, uint32_t initval);
uint32_t rsf_hashbig(const void *key, size_t length, uint32_t initval);
void rsf_hashword2(const uint32_t *key, size_t length, uint32_t *pc, uint32_t *pb);
void rsf_hashlittle2(const void *key, size_t length, uint32_t *pc, uint32_t *pb);

#endif /* RSF_LOOKUP3_H */
