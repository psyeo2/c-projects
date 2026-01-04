#ifndef SELECTION_SORT_H
#define SELECTION_SORT_H

typedef struct
{
    int *list;
    int i;
    int j;
    int min_idx;
    int done;
} SelectionState;

void selection_sort(int *list, int list_length);

SelectionState init_selection_state(int *list);

void step_selection_state(SelectionState *s, int list_length);

#endif