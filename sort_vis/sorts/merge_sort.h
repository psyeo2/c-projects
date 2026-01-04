#ifndef MERGE_SORT_H
#define MERGE_SORT_H

typedef struct
{
    int *list;
    int init;
    int build_i;
    int merge_i;
    int is_sifting;
    int sort_i;
    int is_mergeing;
    int l_idx;
    int r_idx;
    int biggest_idx;
    int done;
} MergeState;

void merge_sort(int *list, int list_length);

MergeState init_merge_state(int *list, int list_length);

void step_merge_state(MergeState *s, int list_length);

#endif