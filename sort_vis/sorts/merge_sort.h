#ifndef MERGE_SORT_H
#define MERGE_SORT_H

typedef struct
{
    int *list;
    int *tmp;
    int sort_i;
    int sort_j;
    int mid;
    int right;
    int merging;
    int merge_i;
    int merge_j;
    int merge_k;
    int merge_l;
    int done;
} MergeState;

void merge_sort(int *list, int list_length);

MergeState init_merge_state(int *list, int list_length);

void step_merge_state(MergeState *s, int list_length);

#endif