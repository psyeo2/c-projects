#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WIDTH 900
#define HEIGHT 600
#define CELL_SIZE 10

typedef enum
{
    INIT,
    RUNNING,
    PAUSED,
    QUIT
} State;

void activate_cell(int *p_grid, int cols, Sint32 x, Sint32 y)
{
    int c = x / CELL_SIZE + 1;
    int r = y / CELL_SIZE + 1;
    p_grid[r * cols + c] = 1;
}

void draw_grid_lines(SDL_Surface *p_surface, Uint32 colour, int rows, int cols)
{
    SDL_Rect horizontal_line;
    SDL_Rect vertical_line;

    horizontal_line.w = WIDTH;
    horizontal_line.h = 1;
    horizontal_line.x = 0;

    vertical_line.w = 1;
    vertical_line.h = HEIGHT;
    vertical_line.y = 0;

    for (int r = 0; r < rows - 1; r++)
    {
        horizontal_line.y = CELL_SIZE * r;
        SDL_FillRect(p_surface, &horizontal_line, colour);
    }
    for (int c = 0; c < cols - 1; c++)
    {
        vertical_line.x = CELL_SIZE * c;
        SDL_FillRect(p_surface, &vertical_line, colour);
    }
}

int get_cell_status(int cols, int *p_grid, int r, int c)
{
    int status = 0;
    int neighbours =
        p_grid[(r-1)*cols + c-1] + p_grid[(r-1)*cols + c] + p_grid[(r-1)*cols + c+1] +
        p_grid[r*cols + c-1]     +           0            +     p_grid[r*cols + c+1] +
        p_grid[(r+1)*cols + c-1] + p_grid[(r+1)*cols + c] + p_grid[(r+1)*cols + c+1];

    switch (neighbours)
    {
    case 0:
    case 1:
        status = 0;
        break;
    case 2:
        status = p_grid[r * cols + c];
        break;
    case 3:
        status = 1;
        break;
    default:
        status = 0;
        break;
    }

    return status;
}

void update_grid(int rows, int cols, int *p_prev_grid, int *p_next_grid)
{
    memset(p_next_grid, 0, rows * cols * sizeof(int));

    for (int r = 1; r < rows - 1; r++)
    {
        for (int c = 1; c < cols - 1; c++)
        {
            p_next_grid[r * cols + c] = get_cell_status(cols, p_prev_grid, r, c);
        }
    }
}

void display_grid(SDL_Surface *p_surface, Uint32 colour, int rows, int cols, int *p_grid)
{
    for (int r = 1; r < rows - 1; r++)
    {
        for (int c = 1; c < cols - 1; c++)
        {
            if (p_grid[r * cols + c])
            {
                SDL_Rect cell_rect = (SDL_Rect){
                    (c - 1) * CELL_SIZE,
                    (r - 1) * CELL_SIZE,
                    CELL_SIZE,
                    CELL_SIZE};
                SDL_FillRect(p_surface, &cell_rect, colour);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    printf("Controls:\nFill cell: <left click>\nToggle grid: <g>\nRandom fill grid: <r>\nPlay/pause: <space>\nExit: <esc>\n");
    long fps;
    if (argc != 2 || !(fps = atol(argv[1])))
    {
        printf("Usage: ./game_of_life <?updates-per-second>\n");
        fps = 60;
    }
    int frame_delay = (int)(1000 / fps);

    if(TTF_Init() != 0)
    {
        printf("TTF Error\n");
        return 1;
    }
    TTF_Font *font = TTF_OpenFont("font.ttf", 16);
    if (!font)
    {
        printf("Load font error: %s\n", TTF_GetError());
        return 1;
    }

    srand(time(NULL));

    SDL_Window *p_window = SDL_CreateWindow(
        "Conway's Game of Life",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH,
        HEIGHT,
        0);

    SDL_Surface *p_surface = SDL_GetWindowSurface(p_window);

    Uint32 black = SDL_MapRGB(p_surface->format, 0, 0, 0);
    // Uint32 white = SDL_MapRGB(p_surface->format, 255, 255, 255);
    Uint32 green = SDL_MapRGB(p_surface->format, 0, 255, 0);
    Uint32 grey = SDL_MapRGB(p_surface->format, 45, 45, 45);

    int cols = WIDTH / CELL_SIZE + 2;
    int rows = HEIGHT / CELL_SIZE + 2;

    int prev_grid[rows * cols];
    int next_grid[rows * cols];
    memset(prev_grid, 0, sizeof(prev_grid));
    memset(next_grid, 0, sizeof(next_grid));

    int *prev = &prev_grid[0];
    int *next = &next_grid[0];
    int *tmp;

    SDL_Event event;

    int b_draw_grid_lines = 0;

    State state = INIT;

    char *state_text = "Initialising";

    while (state != QUIT)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                state_text = "Quitting";
                state = QUIT;
            }
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_g)
                {
                    b_draw_grid_lines ^= 1;
                }
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    state_text = "Quitting";
                    state = QUIT;
                }

                if (state == INIT && event.key.keysym.sym == SDLK_r)
                {
                    for (int r = 0; r < rows; r++)
                    {
                        for (int c = 0; c < cols; c++)
                        {
                            if ((double)rand() / RAND_MAX > 0.8)
                                prev_grid[r * cols + c] = 1;
                        }
                    }
                } else if ((state == RUNNING || state == PAUSED) && event.key.keysym.sym == SDLK_r)
                {
                    memset(prev_grid, 0, sizeof(prev_grid));
                    memset(next_grid, 0, sizeof(next_grid));

                    prev = &prev_grid[0];
                    next = &next_grid[0];

                    state_text = "Initialising";
                    state = INIT;
                }
                if (state == INIT && event.key.keysym.sym == SDLK_SPACE)
                {
                    state_text = "Running";
                    state = RUNNING;
                }
                else if ((state == RUNNING || state == PAUSED) && event.key.keysym.sym == SDLK_SPACE)
                {
                    state = (state == RUNNING) ? PAUSED : RUNNING;
                    state_text = (state == RUNNING) ? "Running" : "Paused";
                }
            }
            if (state == INIT && event.type == SDL_MOUSEBUTTONDOWN)
            {
                Sint32 x = event.button.x;
                Sint32 y = event.button.y;
                activate_cell(prev, cols, x, y);
            }
        }

        SDL_FillRect(p_surface, NULL, black);

        if (b_draw_grid_lines)
            draw_grid_lines(p_surface, grey, rows, cols);

        if (state == RUNNING)
        {
            update_grid(rows, cols, prev, next);
            tmp = prev;
            prev = next;
            next = tmp;
        }

        display_grid(p_surface, green, rows, cols, prev);

        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface *text_surface = TTF_RenderText_Blended(font, state_text, white);
        SDL_Rect dst = {10, 10, text_surface->w, text_surface->h};
        SDL_BlitSurface(text_surface, NULL, p_surface, &dst);

        SDL_UpdateWindowSurface(p_window);
        SDL_Delay(state == INIT ? 16 : frame_delay);
        SDL_FreeSurface(text_surface);
    }

    return 0;
}
