#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>

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
    SELECTION
} SortType;

typedef struct
{
    int *list;
    int i;
    int j;
    int swapped;
    int done;
} BubbleState;

typedef struct
{
    int *list;
    int i;
    int j;
    int key;
    int done;
} InsertionState;

typedef struct
{
    int *list;
    int i;
    int j;
    int min_idx;
    int done;
} SelectionState;

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

void bubble_sort(int *list)
{
    for (int i = BARS; i > 0; i--)
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

void apparently_not_insertion_sort(int *list)
{
    for (int i = 1; i < BARS; i++)
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

void insertion_sort(int *list)
{
    for (int i = 1; i < BARS; i++)
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

void selection_sort(int *list)
{
    for (int i = 0; i < BARS; i++)
    {
        int min_idx = i;
        for (int j = i + 1; j < BARS; j++)
        {
            if (list[j] < list[min_idx])
                min_idx = j;
        }
        int tmp = list[i];
        list[i] = list[min_idx];
        list[min_idx] = tmp;
    }
}

BubbleState init_bubble_state(int *list)
{
    BubbleState s;
    s.list = list;
    s.i = BARS;
    s.j = 0;
    s.swapped = 0;
    s.done = 0;
    return s;
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

void step_insertion_state(InsertionState *s)
{
    if (s->j >= 0 && s->key < s->list[s->j])
    {
        s->list[s->j + 1] = s->list[s->j];
        s->j--;
    }
    else
    {
        s->list[s->j + 1] = s->key;
        if (s->i < BARS - 1)
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

void step_selection_state(SelectionState *s)
{
    if (s->j < BARS)
    {
        if (s->list[s->j] < s->list[s->min_idx])
            s->min_idx = s->j;
        s->j++;
    }
    else if (s->i < BARS - 1)
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

int main()
{
    srand(time(NULL));

    int *list = create_random_list(100);

    SortType sort_type = SELECTION;

    SDL_Window *p_window = SDL_CreateWindow("Sort Visualiser", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Surface *p_surface = SDL_GetWindowSurface(p_window);

    Uint32 *colours = bar_colours(p_surface);

    SDL_Event event;

    BubbleState b_s = init_bubble_state(list);
    InsertionState i_s = init_insertion_state(list);
    SelectionState s_s = init_selection_state(list);

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
                if (event.key.keysym.sym == SDLK_r)
                {
                    list = create_random_list(100);
                    b_s = init_bubble_state(list);
                    i_s = init_insertion_state(list);
                    s_s = init_selection_state(list);
                }
                if (event.key.keysym.sym == SDLK_b)
                {
                    b_s = init_bubble_state(list);
                    sort_type = BUBBLE;
                }
                if (event.key.keysym.sym == SDLK_i)
                {
                    i_s = init_insertion_state(list);
                    sort_type = INSERTION;
                }
                if (event.key.keysym.sym == SDLK_s)
                {
                    s_s = init_selection_state(list);
                    sort_type = SELECTION;
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
                step_insertion_state(&i_s);
            break;
        case (SELECTION):
            if (!s_s.done)
                step_selection_state(&s_s);
            break;
        }

        render_list(p_surface, list, colours);

        SDL_UpdateWindowSurface(p_window);
        // SDL_Delay(1);
    }

    return 0;
}
