#include "heap_sort.h"

void max_heapify(int *list, int i, int list_length)
{
    int left = 2 * i + 1;
    int right = 2 * i + 2;
    int max;

    if (left <= list_length - 1 && list[left] > list[i])
    {
        max = left;
    }
    else
    {
        max = i;
    }

    if (right <= list_length - 1 && list[right] > list[max])
    {
        max = right;
    }

    if (max != i)
    {
        int tmp = list[i];
        list[i] = list[max];
        list[max] = tmp;
        max_heapify(list, max, list_length);
    }
}

void build_max_heap(int *list, int list_length)
{
    for (int i = list_length / 2 - 1; i >= 0; i--)
    {
        max_heapify(list, i, list_length);
    }
}

void heap_sort(int *list, int list_length)
{
    build_max_heap(list, list_length);
    for (int i = list_length - 1; i > 0; i--)
    {
        int tmp = list[0];
        list[0] = list[i];
        list[i] = tmp;
        
        max_heapify(list, 0, i);
    }
}