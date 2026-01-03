#ifndef BUBBLE_SORT_H
#define BUBBLE_SORT_H

typedef struct
{
    int *list;
    int i;
    int j;
    int swapped;
    int done;
} BubbleState;

BubbleState init_bubble_state(int *list, int list_length);

void step_bubble_state(BubbleState *s);

#endif