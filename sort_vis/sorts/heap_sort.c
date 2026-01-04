#include "heap_sort.h"

void explicit_max_heapify(int *list, int i, int list_length)
{
    if (2 * i + 1 >= list_length)
        return;

    int r_exists = 0;

    if (2 * i + 2 < list_length)
    {
        r_exists = 1;
    }

    int biggest_idx;
    if (r_exists && list[2 * i + 1] >= list[2 * i + 2])
    {
        biggest_idx = 2 * i + 1;
    }
    else if (r_exists)
    {
        biggest_idx = 2 * i + 2;
    }
    else
    {
        biggest_idx = 2 * i + 1;
    }

    if (list[biggest_idx] > list[i])
    {
        int tmp = list[i];
        list[i] = list[biggest_idx];
        list[biggest_idx] = tmp;
        max_heapify(list, biggest_idx, list_length);
    }
}

void max_heapify(int *list, int i, int list_length)
{
    int l_idx = 2 * i + 1;
    int r_idx = 2 * i + 2;
    if (l_idx >= list_length)
        return;

    int biggest_idx = l_idx;
    if (r_idx < list_length && list[r_idx] > list[l_idx])
    {
        biggest_idx = r_idx;
    }

    if (list[biggest_idx] > list[i])
    {
        int tmp = list[i];
        list[i] = list[biggest_idx];
        list[biggest_idx] = tmp;
        max_heapify(list, biggest_idx, list_length);
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

HeapState init_heap_state(int *list, int list_length)
{
    HeapState s;
    s.list = list;
    s.build_i = list_length / 2 - 1;
    s.heap_i = 0;
    s.is_sifting = 0;
    s.sort_i = list_length - 1;
    s.is_heaping = 0;
    s.l_idx = 2 * s.build_i + 1;
    s.r_idx = 2 * s.build_i + 2;
    s.biggest_idx = s.l_idx;
    s.init = 0;
    s.done = 0;
    return s;
}

// void step_heapify(HeapState *s, int list_length)
// {
// }

// to do: rip out duplicate heapify logic into above function
// can prob get rid of some of the state flags as well in that case
void step_heap_state(HeapState *s, int list_length)
{
    if (s->done)
        return;
    if (!s->init)
    {
        if (!s->is_sifting)
        {
            if (s->build_i < 0)
            {
                s->init = 1;
                return;
            }
            s->heap_i = s->build_i;
            s->is_sifting = 1;
            return;
        }

        s->l_idx = 2 * s->heap_i + 1;
        s->r_idx = 2 * s->heap_i + 2;

        if (s->l_idx >= list_length)
        {
            s->is_sifting = 0;
            s->build_i--;
            return;
        }

        s->biggest_idx = s->l_idx;
        if (s->r_idx < list_length && s->list[s->r_idx] > s->list[s->l_idx])
        {
            s->biggest_idx = s->r_idx;
        }
        if (s->list[s->biggest_idx] > s->list[s->heap_i])
        {
            int tmp = s->list[s->heap_i];
            s->list[s->heap_i] = s->list[s->biggest_idx];
            s->list[s->biggest_idx] = tmp;
            s->heap_i = s->biggest_idx;
        }
        else
        {
            s->is_sifting = 0;
            s->build_i--;
        }
    }
    else
    {
        if (s->sort_i <= 0)
        {
            s->done = 1;
            return;
        }
        if (!s->is_heaping)
        {
            int tmp = s->list[0];
            s->list[0] = s->list[s->sort_i];
            s->list[s->sort_i] = tmp;
            s->heap_i = 0;
            s->is_heaping = 1;
        }
        else
        {
            s->l_idx = 2 * s->heap_i + 1;
            s->r_idx = 2 * s->heap_i + 2;
            if (s->l_idx >= s->sort_i)
            {
                s->is_heaping = 0;
                s->sort_i--;
                return;
            }

            s->biggest_idx = s->l_idx;
            if (s->r_idx < s->sort_i && s->list[s->r_idx] > s->list[s->l_idx])
                s->biggest_idx = s->r_idx;

            if (s->list[s->biggest_idx] > s->list[s->heap_i])
            {
                int tmp = s->list[s->heap_i];
                s->list[s->heap_i] = s->list[s->biggest_idx];
                s->list[s->biggest_idx] = tmp;
                s->heap_i = s->biggest_idx;
            }
            else
            {
                s->is_heaping = 0;
                s->sort_i--;
                return;
            }
        }
    }
}