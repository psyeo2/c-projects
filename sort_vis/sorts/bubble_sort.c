#include "bubble_sort.h"

void bubble_sort(int *list, int list_length)
{
    for (int i = list_length; i > 0; i--)
    {
        int swapped = 0;
        for (int j = 0; j < i - 1; j++)
        {
            if (list[j] > list[j + 1])
            {
                int tmp = list[j];
                list[j] = list[j + 1];
                list[j + 1] = tmp;
                swapped = 1;
            }
        }
        if (!swapped)
            break;
    }
}

BubbleState init_bubble_state(int *list, int list_length)
{
    BubbleState s;
    s.list = list;
    s.i = list_length;
    s.j = 0;
    s.swapped = 0;
    s.done = 0;
    return s;
}

void step_bubble_state(BubbleState *s)
{
    if (s->j < s->i - 1 && s->list[s->j] > s->list[s->j + 1])
    {
        int tmp = s->list[s->j];
        s->list[s->j] = s->list[s->j + 1];
        s->list[s->j + 1] = tmp;
        s->swapped = 1;
    }

    if (s->j < s->i - 1)
    {
        s->j++;
    }

    if (s->j == s->i - 1 && s->swapped && s->i > 1)
    {
        s->j = 0;
        s->i--;
        s->swapped = 0;
    }
    else if (s->j == s->i - 1 && !s->swapped)
    {
        s->done = 1;
    }
    else
    {
        s->done = 0;
    }
}