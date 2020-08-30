#pragma once
#include <stdbool.h>
#include <stdlib.h>

typedef struct BHeap_s BHeap;
typedef int(*BHeapNodeCompareFunc)(void *a, void*b);
typedef void(*BHeapNodeFreeFunc)(void *node);

BHeap *b_heap_new(BHeapNodeCompareFunc cmp);
BHeap *b_heap_new_full(BHeapNodeCompareFunc cmp, BHeapNodeFreeFunc node_free);
void b_heap_free(BHeap *self);

void b_heap_push(BHeap *self, void *data);
void *b_heap_pop(BHeap *self);
bool b_heap_is_empty(BHeap *self);

typedef void (*BHeapTraverseFunc)(void *data);
void b_heap_traverse(BHeap *self, BHeapTraverseFunc fn);
void b_heap_print(BHeap *self, BHeapTraverseFunc fn);
