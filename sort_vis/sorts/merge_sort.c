#include "merge_sort.h"

#include <stdlib.h>

void merge(int *list, int *a, int *b, int len_a, int len_b)
{
    int a_idx = 0;
    int b_idx = 0;
    for(int i = 0; i < len_a + len_b; i++)
    {
        if(a_idx < len_a && b_idx < len_b)
        {
            if(a[a_idx] < b[b_idx])
            {
                list[i] = a[a_idx];
                a_idx++;
            }
            else
            {
                list[i] = b[b_idx];
                b_idx++;
            }
        }
        else if (a_idx < len_a)
        {
            list[i] = a[a_idx];
            a_idx++;
        }
        else if (b_idx < len_b)
        {
            list[i] = b[b_idx];
            b_idx++;
        }
        else
        {
            return;
        }
    }
}

void merge_sort(int *list, int list_length)
{
    if (list_length < 2)
        return;

    int mid = list_length / 2;

    int *a = malloc(mid * sizeof(int));
    int *b = malloc((list_length - mid) * sizeof(int));
    if (!a || !b)
    {
        free(a);
        free(b);
        return;
    }

    for (int i = 0; i < mid; i++)
    {
        a[i] = list[i];
    }
    for (int i = mid; i < list_length; i++)
    {
        b[i - mid] = list[i];
    }

    merge_sort(a, mid);
    merge_sort(b, list_length - mid);

    merge(list, a, b, mid, list_length - mid);

    free(a);
    free(b);
}