#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>

#include "sorts/bubble_sort.h"
#include "sorts/insertion_sort.h"
#include "sorts/selection_sort.h"
#include "sorts/shell_sort.h"
#include "sorts/heap_sort.h"
#include "sorts/merge_sort.h"

#define WIDTH 900
#define HEIGHT 600

#define BAR_WIDTH 3
#define BARS (WIDTH / BAR_WIDTH)

typedef enum
{
    INIT,
    RUNNING,
    PAUSED,
    QUIT
} State;

typedef enum
{
    BUBBLE,
    INSERTION,
    SELECTION,
    SHELL,
    HEAP,
    MERGE
} SortType;

int rand_in_range(int min, int max)
{
    return min + rand() % (max - min + 1);
}

int *create_random_list(int iterations)
{
    int *ret = malloc(BARS * sizeof(int));

    for (int i = 0; i < BARS; i++)
    {
        ret[i] = i + 1;
    }

    for (int i = 0; i < iterations; i++)
    {
        for (int j = BARS - 1; j > 0; j--)
        {
            int k = rand_in_range(0, j);
            int tmp = ret[j];
            ret[j] = ret[k];
            ret[k] = tmp;
        }
    }

    return ret;
}

Uint32 *bar_colours(SDL_Surface *p_surface)
{
    Uint32 *colours = malloc(BARS * sizeof(Uint32));
    for (int i = 0; i < BARS; i++)
    {
        colours[i] = SDL_MapRGB(p_surface->format, rand_in_range(50, 255), rand_in_range(50, 255), rand_in_range(50, 255));
    }

    return colours;
}

void render_list(SDL_Surface *p_surface, int *list, Uint32 *colours)
{
    SDL_Rect bar;
    bar.w = BAR_WIDTH;
    for (int i = 0; i < BARS; i++)
    {
        bar.x = i * BAR_WIDTH;
        double height = (list[i] / (double)BARS) * HEIGHT;
        bar.h = (int)height;
        bar.y = HEIGHT - bar.h;
        SDL_FillRect(p_surface, &bar, colours[list[i] - 1]);
    }
}

int main()
{
    srand(time(NULL));
    printf("r: reset\nb: bubble sort\ni: insertion sort\ns: selection sort\nl: shell sort\nh: heap sort\nm: merge sort\n");

    int *list = create_random_list(100);
    // merge_sort(list, BARS);

    SortType sort_type = BUBBLE;

    SDL_Window *p_window = SDL_CreateWindow("Sort Visualiser", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Surface *p_surface = SDL_GetWindowSurface(p_window);

    Uint32 *colours = bar_colours(p_surface);

    SDL_Event event;

    int list_length = BARS;

    BubbleState b_s = init_bubble_state(list, list_length);
    InsertionState i_s = init_insertion_state(list);
    SelectionState se_s = init_selection_state(list);
    ShellState sh_s = init_shell_state(list, list_length);
    HeapState h_s = init_heap_state(list, list_length);
    MergeState m_s = init_merge_state(list, list_length);

    Uint32 black = SDL_MapRGB(p_surface->format, 0, 0, 0);
    State state = RUNNING;
    while (state != QUIT)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
            {
                state = QUIT;
            }
            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_r:
                    list = create_random_list(100);
                    b_s = init_bubble_state(list, list_length);
                    i_s = init_insertion_state(list);
                    se_s = init_selection_state(list);
                    sh_s = init_shell_state(list, list_length);
                    h_s = init_heap_state(list, list_length);
                    m_s = init_merge_state(list, list_length);
                    break;
                case SDLK_b:
                    b_s = init_bubble_state(list, list_length);
                    sort_type = BUBBLE;
                    break;
                case SDLK_i:
                    i_s = init_insertion_state(list);
                    sort_type = INSERTION;
                    break;
                case SDLK_s:
                    se_s = init_selection_state(list);
                    sort_type = SELECTION;
                    break;
                case SDLK_l:
                    sh_s = init_shell_state(list, list_length);
                    sort_type = SHELL;
                    break;
                case SDLK_h:
                    h_s = init_heap_state(list, list_length);
                    sort_type = HEAP;
                    break;
                case SDLK_m:
                    m_s = init_merge_state(list, list_length);
                    sort_type = MERGE;
                    break;
                default:
                    break;
                }
            }
        }
        SDL_FillRect(p_surface, NULL, black);

        switch (sort_type)
        {
        case (BUBBLE):
            if (!b_s.done)
                step_bubble_state(&b_s);
            break;
        case (INSERTION):
            if (!i_s.done)
                step_insertion_state(&i_s, list_length);
            break;
        case (SELECTION):
            if (!se_s.done)
                step_selection_state(&se_s, list_length);
            break;
        case (SHELL):
            if (!sh_s.done)
                step_shell_state(&sh_s, list_length);
            break;
        case (HEAP):
            if (!h_s.done)
                step_heap_state(&h_s, list_length);
            break;
        case (MERGE):
            if (!m_s.done)
                step_merge_state(&m_s, list_length);
            break;
        }

        render_list(p_surface, list, colours);

        SDL_UpdateWindowSurface(p_window);
        // SDL_Delay(1);
    }

    return 0;
}
