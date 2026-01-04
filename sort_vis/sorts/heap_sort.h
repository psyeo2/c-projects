#ifndef HEAP_SORT_H
#define HEAP_SORT_H

typedef struct
{
    int *list;
    int i;
    int j;
    int key;
    int done;
} HeapState;

void max_heapify(int *list, int i, int list_length);

void build_max_heap(int *list, int list_length);

void heap_sort(int *list, int list_length);

// HeapState init_heap_state(int *list);

// void step_heap_state(HeapState *s, int list_length);

#endif