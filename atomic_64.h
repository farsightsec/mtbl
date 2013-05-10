#ifndef MY_ATOMIC_64_H
#define MY_ATOMIC_64_H

#include <stdint.h>

/* public domain, from http://golubenco.org/2007/06/14/atomic-operations/ */

/**
 * Atomic 64-bit type.
 */

typedef struct {
    volatile int64_t counter;
} atomic64_t;

#define ATOMIC64_INIT(i)  { (i) }

/**
 * Read atomic variable
 * @param v pointer of type atomic64_t
 *
 * Atomically reads the value of @v.
 */
#define atomic64_read(v) ((v)->counter)

/**
 * Set atomic variable
 * @param v pointer of type atomic64_t
 * @param i required value
 */
#define atomic64_set(v, i) (((v)->counter) = (i))

/**
 * Add to the atomic variable
 * @param i integer value to add
 * @param v pointer of type atomic64_t
 */
static inline void
atomic64_add(int i, atomic64_t *v) {
         (void)__sync_add_and_fetch(&v->counter, i);
}

/**
 * Subtract the atomic variable
 * @param i integer value to subtract
 * @param v pointer of type atomic64_t
 *
 * Atomically subtracts @i from @v.
 */
static inline void
atomic64_sub(int i, atomic64_t *v) {
        (void)__sync_sub_and_fetch(&v->counter, i);
}

/**
 * Read atomic variable and reset to zero.
 *
 * @param v pointer of type atomic64_t
 */
static inline int
atomic64_zero(atomic64_t *v) {
	return (__sync_fetch_and_and(&v->counter, 0));
}

/**
 * Subtract value from variable and test result
 * @param i integer value to subtract
 * @param v pointer of type atomic64_t
 *
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.
 */
static inline int
atomic64_sub_and_test(int i, atomic64_t *v) {
        return !(__sync_sub_and_fetch(&v->counter, i));
}

/**
 * Increment atomic variable
 * @param v pointer of type atomic64_t
 *
 * Atomically increments @v by 1.
 */
static inline void
atomic64_inc(atomic64_t *v) {
       (void)__sync_fetch_and_add(&v->counter, 1);
}

/**
 * @brief decrement atomic variable
 * @param v: pointer of type atomic64_t
 *
 * Atomically decrements @v by 1.  Note that the guaranteed
 * useful range of an atomic64_t is only 24 bits.
 */
static inline void
atomic64_dec(atomic64_t *v) {
       (void)__sync_fetch_and_sub(&v->counter, 1);
}

/**
 * @brief Decrement and test
 * @param v pointer of type atomic64_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.
 */
static inline int
atomic64_dec_and_test(atomic64_t *v) {
       return !(__sync_sub_and_fetch(&v->counter, 1));
}

/**
 * @brief Increment and test
 * @param v pointer of type atomic64_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
static inline int
atomic64_inc_and_test(atomic64_t *v) {
      return !(__sync_add_and_fetch(&v->counter, 1));
}

/**
 * @brief add and test if negative
 * @param v pointer of type atomic64_t
 * @param i integer value to add
 *
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.
 */
static inline int
atomic64_add_negative(int i, atomic64_t *v) {
       return (__sync_add_and_fetch(&v->counter, i) < 0);
}

#endif /* MY_ATOMIC_H */
