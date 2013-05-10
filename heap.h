#ifndef MY_HEAP_H
#define MY_HEAP_H

struct heap;

typedef int (*heap_compare_func)(const void *a, const void *b);

struct heap *heap_init(heap_compare_func);
void heap_destroy(struct heap **);
void heap_push(struct heap *, void *);
void *heap_pop(struct heap *);
void *heap_replace(struct heap *, void *);
void *heap_peek(struct heap *);
void *heap_get(struct heap *, size_t);
size_t heap_size(struct heap *);

#endif /* MY_HEAP_H */
