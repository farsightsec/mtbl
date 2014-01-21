#ifndef MY_ATOMIC_PTR_H
#define MY_ATOMIC_PTR_H

typedef struct {
    volatile void *ptr;
} atomic_ptr_t;

static inline void *
atomic_ptr_get(atomic_ptr_t *v) {
	return ((void *) (__sync_fetch_and_add(&v->ptr, 0)));
}

static inline bool
atomic_ptr_set(atomic_ptr_t *v, void *new_ptr) {
	void *old_ptr = atomic_ptr_get(v);
	return (__sync_bool_compare_and_swap(&v->ptr, old_ptr, new_ptr));
}

#endif /* MY_ATOMIC_PTR_H */
