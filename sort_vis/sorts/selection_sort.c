#include "selection_sort.h"

void selection_sort(int *list, int list_length)
{
    for (int i = 0; i < list_length; i++)
    {
        int min_idx = i;
        for (int j = i + 1; j < list_length; j++)
        {
            if (list[j] < list[min_idx])
                min_idx = j;
        }
        int tmp = list[i];
        list[i] = list[min_idx];
        list[min_idx] = tmp;
    }
}

SelectionState init_selection_state(int *list)
{
    SelectionState s;
    s.list = list;
    s.i = 0;
    s.j = s.i;
    s.min_idx = s.i;
    s.done = 0;
    return s;
}

void step_selection_state(SelectionState *s, int list_length)
{
    if (s->j < list_length)
    {
        if (s->list[s->j] < s->list[s->min_idx])
            s->min_idx = s->j;
        s->j++;
    }
    else if (s->i < list_length - 1)
    {
        int tmp = s->list[s->i];
        s->list[s->i] = s->list[s->min_idx];
        s->list[s->min_idx] = tmp;

        s->i++;
        s->min_idx = s->i;
        s->j = s->i + 1;
    }
    else
    {
        s->done = 1;
    }
}