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

typedef struct
{
    int *list;
    int i;
    int j;
    int swapped;
} BubbleState;

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
        SDL_FillRect(p_surface, &bar, colours[list[i]] - 1);
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

BubbleState init_bubble_state(int *list)
{
    BubbleState s;
    s.list = list;
    s.i = BARS;
    s.j = 0;
    s.swapped = 0;
    return s;
}

int step_bubble_state(BubbleState *s)
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

    if (s->j == s->i - 2 && s->swapped && s->i <= 1)
    {
        s->j = 0;
        s->i--;
        s->swapped = 0;
    }
    else if (s->j == s->i - 1 && !s->swapped)
    {
        return 1;
    }

    return 0;
}

int main()
{
    srand(time(NULL));

    int *list = create_random_list(100);

    SDL_Window *p_window = SDL_CreateWindow("Sort Visualiser", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Surface *p_surface = SDL_GetWindowSurface(p_window);

    Uint32 *colours = bar_colours(p_surface);

    SDL_Event event;

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
        }
        SDL_FillRect(p_surface, NULL, black);

        BubbleState s = init_bubble_state(list);

        render_list(p_surface, list, colours);

        SDL_UpdateWindowSurface(p_window);
        SDL_Delay(16);
    }

    return 0;
}
