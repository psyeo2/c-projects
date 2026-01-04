#include "merge_sort.h"

#include <stdlib.h>

void merge(int *list, int *a, int *b, int len_a, int len_b)
{
    int a_idx = 0;
    int b_idx = 0;
    for (int i = 0; i < len_a + len_b; i++)
    {
        if (a_idx < len_a && b_idx < len_b)
        {
            if (a[a_idx] < b[b_idx])
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

void recursive_merge_sort(int *list, int list_length)
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

void merge_range(int *list, int *tmp, int l_idx, int mid_idx, int r_idx)
{
    int i = l_idx;
    int j = mid_idx;

    for (int k = l_idx; k < r_idx; k++)
    {
        if (i < mid_idx && j < r_idx)
        {
            if (list[i] <= list[j])
            {
                tmp[k] = list[i];
                i++;
            }
            else
            {
                tmp[k] = list[j];
                j++;
            }
        }
        else if (i < mid_idx)
        {
            tmp[k] = list[i];
            i++;
        }
        else if (j < r_idx)
        {
            tmp[k] = list[j];
            j++;
        }
    }

    for (int k = l_idx; k < r_idx; k++)
        list[k] = tmp[k];
}

void merge_sort(int *list, int list_length)
{
    int *tmp = malloc(list_length * sizeof(int));
    for (int i = 1; i < list_length; i *= 2)
    {
        for (int j = 0; j < list_length; j += 2 * i)
        {
            int mid = j + i;
            int right = j + 2 * i;
            if (mid > list_length)
                mid = list_length;
            if (right > list_length)
                right = list_length;

            merge_range(list, tmp, j, mid, right);
        }
    }
    free(tmp);
}
