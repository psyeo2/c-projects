#ifndef INSERTION_SORT_H
#define INSERTION_SORT_H

typedef struct
{
    int *list;
    int i;
    int j;
    int key;
    int done;
} InsertionState;

InsertionState init_insertion_state(int *list);

void step_insertion_state(InsertionState *s, int list_length);

#endif