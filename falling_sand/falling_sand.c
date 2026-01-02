#include <stdio.h>
#include <SDL2/SDL.h>
#include <time.h>

#define WIDTH 900
#define HEIGHT 600
#define CELL_SIZE 2

typedef enum
{
    AIR,
    WALL,
    SAND,
    WATER
} Material;

typedef enum
{
    INIT,
    RUNNING,
    PAUSED,
    QUIT
} State;

void modify_sand(int radius, Sint32 x, Sint32 y, int *p_cells, int rows, int cols, int create)
{
    int cell_x = x / CELL_SIZE;
    int cell_y = y / CELL_SIZE;

    int r0 = cell_y - radius;
    int r1 = cell_y + radius;
    int c0 = cell_x - radius;
    int c1 = cell_x + radius;

    for (int r = r0; r <= r1; r++)
    {
        if (r < 0 || r >= rows)
            continue;

        int dy = r - cell_y;
        for (int c = c0; c <= c1; c++)
        {
            if (c < 0 || c >= cols)
                continue;

            int dx = c - cell_x;
            if (dx * dx + dy * dy <= radius * radius)
            {
                p_cells[r * cols + c] = create;
            }
        }
    }
}

void update_cells(int *p_cells, int *p_next_cells, int rows, int cols)
{
    memset(p_next_cells, 0, rows * cols * sizeof(int));
    for (int c = 0; c < cols; c++)
    {
        p_next_cells[(rows - 1) * cols + c] = 1;
    }

    for (int r = rows - 2; r >= 0; r--)
    {
        int start = (r & 1) ? cols - 1 : 0;
        int step = (r & 1) ? -1 : 1;

        for (int c = start; c >= 0 && c < cols; c += step)
        {
            if (!p_cells[r * cols + c])
                continue;

            if (!p_cells[(r + 1) * cols + c] && !p_next_cells[(r + 1) * cols + c])
            {
                p_cells[r * cols + c] = 0;
                p_next_cells[(r + 1) * cols + c] = 1;
            }
            else if (c + 1 < cols && !p_cells[(r + 1) * cols + c + 1] && !p_next_cells[(r + 1) * cols + c + 1])
            {
                p_cells[r * cols + c] = 0;
                p_next_cells[(r + 1) * cols + c + 1] = 1;
            }
            else if (c - 1 >= 0 && !p_cells[(r + 1) * cols + c - 1] && !p_next_cells[(r + 1) * cols + c - 1])
            {
                p_cells[r * cols + c] = 0;
                p_next_cells[(r + 1) * cols + c - 1] = 1;
            }
            else
            {
                p_next_cells[r * cols + c] = 1;
            }
        }
    }
}

void draw_sand(SDL_Surface *p_surface, int *cells, int rows, int cols, Uint32 colour)
{
    SDL_Rect cell;
    cell.w = CELL_SIZE;
    cell.h = CELL_SIZE;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            if (cells[r * cols + c])
            {
                cell.x = c * CELL_SIZE;
                cell.y = r * CELL_SIZE;
                SDL_FillRect(p_surface, &cell, colour);
            }
        }
    }
}

int main()
{
    int rows = HEIGHT / CELL_SIZE + 1;
    int cols = WIDTH / CELL_SIZE;

    int *p_cells;
    int *p_next_cells;
    int *tmp = NULL;

    p_cells = calloc(rows*cols, sizeof(int));
    p_next_cells = calloc(rows*cols, sizeof(int));

    for (int c = 0; c < cols; c++)
    {
        p_cells[(rows - 1) * cols + c] = 1;
        p_next_cells[(rows - 1) * cols + c] = 1;
    }

    srand(time(NULL));

    SDL_Window *p_window = SDL_CreateWindow("Falling Sand", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Surface *p_surface = SDL_GetWindowSurface(p_window);

    Uint32 black = SDL_MapRGB(p_surface->format, 0, 0, 0);
    Uint32 claret = SDL_MapRGB(p_surface->format, 200, 10, 10);

    int placing_sand = 0;
    int destroying_sand = 0;
    Uint8 button = 0;
    Sint32 mouse_x = 0;
    Sint32 mouse_y = 0;
    SDL_Event event;
    State state = RUNNING;
    while (state != QUIT)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
            {
                state = QUIT;
            }
            if (event.type == SDL_MOUSEMOTION)
            {
                mouse_x = event.motion.x;
                mouse_y = event.motion.y;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                button = event.button.button;
                if (button == 1)
                {
                    placing_sand = 1;
                }
                else if (button == 3)
                {
                    destroying_sand = 1;
                }
            }
            if (event.type == SDL_MOUSEBUTTONUP)
            {
                button = event.button.button;
                if (button == 1)
                {
                    placing_sand = 0;
                }
                else if (button == 3)
                {
                    destroying_sand = 0;
                }
            }
            if (event.type == SDL_KEYDOWN)
            {
            }
        }
        SDL_FillRect(p_surface, NULL, black);

        if (placing_sand)
        {
            modify_sand(25, mouse_x, mouse_y, p_cells, rows, cols, 1);
        }
        if (destroying_sand)
        {
            modify_sand(25, mouse_x, mouse_y, p_cells, rows, cols, 0);
        }

        update_cells(p_cells, p_next_cells, rows, cols);
        tmp = p_cells;
        p_cells = p_next_cells;
        p_next_cells = tmp;

        draw_sand(p_surface, p_cells, rows, cols, claret);

        SDL_UpdateWindowSurface(p_window);
        SDL_Delay(8);
    }

    return 0;
}
