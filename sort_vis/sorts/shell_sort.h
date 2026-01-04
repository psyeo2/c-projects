#ifndef SHELL_SORT_H
#define SHELL_SORT_H

typedef struct
{
    int *list;
    int gap;
    int i;
    int j;
    int key;
    int done;
} ShellState;

void shell_sort(int* list, int list_length);

ShellState init_shell_state(int *list, int list_length);

void step_shell_state(ShellState *s, int list_length);

#endif