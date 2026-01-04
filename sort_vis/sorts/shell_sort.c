#include "shell_sort.h"

#include <stdio.h>

void shell_sort_but_actually_just_insertion_sort(int *list, int list_length)
{
    int gap = list_length / 2;
    while (gap > 0)
    {
        for (int i = gap; i < list_length; i += gap)
        {
            int key = list[i];
            int j = i - gap;
            while (j >= 0 && key < list[j])
            {
                list[j + gap] = list[j];
                j -= gap;
            }
            list[j + gap] = key;
        }

        gap /= 2;
    }
}

// gap = list_length / 2
// insertion sort the list of every ngap + i
// repeat increasing i until i = gap
// gap /= 2 and repeate
void naive_shell_sort(int *list, int list_length)
{
    for (int gap = list_length / 2; gap > 0; gap /= 2)
    {
        for (int i = 0; i < gap; i++)
        {
            for (int j = i + gap; j < list_length; j += gap)
            {
                int key = list[j];
                int k = j - gap;
                while (k >= i && key < list[k])
                {
                    list[k + gap] = list[k];
                    k -= gap;
                }
                list[k + gap] = key;
            }
        }
    }
}

// gap = list_length / 2
// for each index i from gap to list_length - 1:
//     take element at i (key)
//     repeatedly shift elements 'gap' positions to the right
//     while they are greater than key
//     until the correct position for key is found
//     insert key there
// gap /= 2 and repeat
//
// e.g., for array length 16, at gap length 4:
// These are the sequences created by a gap length four:
// 0, 4, 8, 12
// 1, 5, 9, 13
// 2, 6, 10, 14
// 3, 7, 11, 15
// For 3, 7, 11, 15:
// i = 3
// i = 7: insert 7 into [3]
// i = 11: insert 11 into [3, 7]
// i = 15: insert 15 into [3, 7, 11]
// so by the time you reach i = 15, 3 7 11 are already sorted.
void shell_sort(int *list, int list_length)
{
    for (int gap = list_length / 2; gap > 0; gap /= 2)
    {
        for (int i = gap; i < list_length; i++)
        {
            int key = list[i];
            int j = i;
            while(j >= gap && key < list[j - gap])
            {
                list[j] = list[j - gap];
                j -= gap;
            }
            list[j] = key;
        }
    }
}

ShellState init_shell_state(int *list, int list_length)
{
    ShellState s;
    s.list = list;
    s.gap = list_length / 2;
    s.i = s.gap;
    s.j = s.i;
    s.key = list[s.i];
    s.done = 0;
    return s;
}

void step_shell_state(ShellState *s, int list_length)
{
    if (s->j >= s->gap && s->key < s->list[s->j - s->gap])
    {
        s->list[s->j] = s->list[s->j - s->gap];
        s->j -= s->gap;
    }
    else if (s->i < list_length - 1)
    {
        s->list[s->j] = s->key;

        s->i++;
        s->key = s->list[s->i];
        s->j = s->i;
    }
    else if (s->gap > 1)
    {
        s->gap /= 2;

        s->i = s->gap;
        s->key = s->list[s->i];
        s->j = s->i;
    }
    else
    {
        s->done = 1;
    }
}