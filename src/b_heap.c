#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SWAP(x, y, T)                                                          \
    do {                                                                       \
        T __TEMP = (x);                                                        \
        (x) = y;                                                               \
        (y) = __TEMP;                                                          \
    } while (0)

typedef int (*BHeapNodeCompareFunc)(void *a, void *b);
typedef void (*BHeapNodeFreeFunc)(void *node);
typedef void (*BHeapTraverseFunc)(void *data);
typedef void *BHeapNode;

typedef struct {
    BHeapNode *data;
    size_t size;
    size_t capacity;
    BHeapNodeCompareFunc compare_nodes;
    BHeapNodeFreeFunc free_node;
} BHeap;

#define BHEAP_DEFAULT_CAPACITY 1024

BHeap *b_heap_new(BHeapNodeCompareFunc cmp)
{
    BHeap *self = malloc(sizeof(*self));
    self->capacity = BHEAP_DEFAULT_CAPACITY;
    self->data = malloc(sizeof(*self->data) * self->capacity);
    self->compare_nodes = cmp;
    self->free_node = free;
    return self;
}

BHeap *b_heap_new_full(BHeapNodeCompareFunc cmp, BHeapNodeFreeFunc node_free)
{
    BHeap *self = b_heap_new(cmp);
    self->free_node = node_free;
    return self;
}

void b_heap_free(BHeap *self)
{
    for (size_t i = 0; i < self->size; i++) {
        self->free_node(self->data[i]);
    }
    free(self->data);
    free(self);
}

void b_heap_sift_up(BHeap *self, size_t index)
{
    size_t parent_index = (index - 1) / 2;
    BHeapNode parent = self->data[parent_index];

    if (parent_index >= self->size) {
        return;
    }

    if (self->compare_nodes(parent, self->data[index]) < 0) {
        SWAP(self->data[parent_index], self->data[index], BHeapNode);
        b_heap_sift_up(self, parent_index);
    }
}

void b_heap_sift_down(BHeap *self, size_t index)
{
    size_t larg_i = index;
    size_t left_i = index * 2 + 1;
    size_t right_i = index * 2 + 2;

    if (left_i < self->size &&
        self->compare_nodes(self->data[left_i], self->data[larg_i]) > 0) {
        larg_i = left_i;
    }
    if (right_i < self->size &&
        self->compare_nodes(self->data[right_i], self->data[larg_i]) > 0) {
        larg_i = right_i;
    }

    if (larg_i != index) {
        SWAP(self->data[index], self->data[larg_i], BHeapNode);
        b_heap_sift_down(self, larg_i);
    }
}

void b_heap_push(BHeap *self, void *data)
{
    if (self->capacity < self->size + 1) {
        self->capacity *= 2;
        self->data = realloc(self->data, sizeof(*self->data) * self->capacity);
    }

    self->data[self->size] = data;
    self->size += 1;
    b_heap_sift_up(self, self->size - 1);
}

void *b_heap_pop(BHeap *self)
{
    if (self->size <= 0) {
        return NULL;
    }

    self->size -= 1;
    SWAP(self->data[self->size], self->data[0], BHeapNode);
    b_heap_sift_down(self, 0);
    return self->data[self->size];
}

bool b_heap_is_empty(BHeap *self) { return self->size <= 0; }

void b_heap_traverse(BHeap *self, BHeapTraverseFunc fn)
{
    if (self->size > 0) {
        fn(self->data[0]);
    }
    for (size_t i = 1; i < self->size; i++) {
        fn(self->data[i]);
    }
}

void b_heap_print(BHeap *self, BHeapTraverseFunc fn)
{
    printf("[");
    if (self->size > 0) {
        fn(self->data[0]);
    }
    for (size_t i = 1; i < self->size; i++) {
        printf(", ");
        fn(self->data[i]);
    }
    printf("]\n");
}
