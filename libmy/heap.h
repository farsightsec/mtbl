#ifndef MY_HEAP_H
#define MY_HEAP_H

struct heap;

typedef int (*heap_compare_func)(const void *a, const void *b);

struct heap *heap_init(heap_compare_func);
void heap_destroy(struct heap **);
void heap_reset(struct heap *);
void heap_clip(struct heap *, size_t);
void heap_heapify(struct heap *);
/* you should use heap_add if you are adding more than n/log(n) elements
 * at a time.  you must call heap_heapify after calling heap_add. this
 * reduces the time complexity from O(n*log(n)) to O(n)*/
void heap_add(struct heap *, void *);
void heap_push(struct heap *, void *);
void *heap_pop(struct heap *);
void *heap_replace(struct heap *, void *);
void *heap_peek(struct heap *);
void *heap_get(struct heap *, size_t);
size_t heap_size(struct heap *);

#endif /* MY_HEAP_H */
