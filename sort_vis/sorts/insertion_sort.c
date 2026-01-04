#include "insertion_sort.h"

void apparently_not_insertion_sort(int *list, int list_length)
{
    for (int i = 1; i < list_length; i++)
    {
        int j = i;
        while (j > 0 && list[j] < list[j - 1])
        {
            int tmp = list[j];
            list[j] = list[j - 1];
            list[j - 1] = tmp;
            j--;
        }
    }
}

void insertion_sort(int *list, int list_length)
{
    for (int i = 1; i < list_length; i++)
    {
        int key = list[i];
        int j = i - 1;
        while (j >= 0 && key < list[j])
        {
            list[j + 1] = list[j];
            j--;
        }
        list[j + 1] = key;
    }
}

InsertionState init_insertion_state(int *list)
{
    InsertionState s;
    s.list = list;
    s.i = 1;
    s.j = s.i - 1;
    s.key = list[s.i];
    s.done = 0;
    return s;
}

void step_insertion_state(InsertionState *s, int list_length)
{
    if (s->j >= 0 && s->key < s->list[s->j])
    {
        s->list[s->j + 1] = s->list[s->j];
        s->j--;
    }
    else
    {
        s->list[s->j + 1] = s->key;
        if (s->i < list_length - 1)
        {
            s->i++;
            s->key = s->list[s->i];
            s->j = s->i - 1;
        }
        else
        {
            s->done = 1;
        }
    }
}