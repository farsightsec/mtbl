#ifndef MY_LOOKUP3_H
#define MY_LOOKUP3_H

#include <stdint.h>

uint32_t my_hashword(const uint32_t *key, size_t length, uint32_t initval);
uint32_t my_hashlittle(const void *key, size_t length, uint32_t initval);
uint32_t my_hashbig(const void *key, size_t length, uint32_t initval);
void my_hashword2(const uint32_t *key, size_t length, uint32_t *pc, uint32_t *pb);
void my_hashlittle2(const void *key, size_t length, uint32_t *pc, uint32_t *pb);

#endif /* MY_LOOKUP3_H */
