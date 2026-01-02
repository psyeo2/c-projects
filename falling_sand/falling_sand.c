#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <time.h>

#define WIDTH 900
#define HEIGHT 600
#define CELL_SIZE 2

typedef enum
{
    AIR,
    BOUNDARY,
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

void modify_cells(int radius, Sint32 x, Sint32 y, Material *p_cells, int rows, int cols, Material material)
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
                if (material != BOUNDARY)
                    p_cells[r * cols + c] = material;
            }
        }
    }
}

void update_cells(Material *p_cells, Material *p_next_cells, int rows, int cols)
{
    // memset(p_next_cells, 0, rows * cols * sizeof(Material));
    // for (int c = 0; c < cols; c++)
    // {
    //     p_next_cells[(rows - 1) * cols + c] = BOUNDARY;
    // }

    int direction = 0;
    for (int r = rows - 2; r >= 0; r--)
    {
        int start = (r & 1) ? cols - 1 : 0;
        int step = (r & 1) ? -1 : 1;

        for (int c = start; c >= 0 && c < cols; c += step)
        {
            Material material = p_cells[r * cols + c];

            if (material == AIR)
                continue;
            if (material == WALL)
            {
                p_next_cells[r * cols + c] = WALL;
                continue;
            }

            direction = ((rand() >> 8) & 1) ? 1 : -1;
            if (material == SAND)
            {
                if (p_cells[(r + 1) * cols + c] == AIR && p_next_cells[(r + 1) * cols + c] == AIR)
                {
                    p_cells[r * cols + c] = AIR;
                    p_next_cells[(r + 1) * cols + c] = SAND;
                }
                else if ((c + 1 < cols && c - 1 >= 0) && p_cells[(r + 1) * cols + c + direction] == AIR && p_next_cells[(r + 1) * cols + c + direction] == AIR)
                {
                    p_cells[r * cols + c] = AIR;
                    p_next_cells[(r + 1) * cols + c + direction] = SAND;
                }
                else if ((c + 1 < cols && c - 1 >= 0) && p_cells[(r + 1) * cols + c - direction] == AIR && p_next_cells[(r + 1) * cols + c - direction] == AIR)
                {
                    p_cells[r * cols + c] = AIR;
                    p_next_cells[(r + 1) * cols + c - direction] = SAND;
                }
                else
                {
                    p_next_cells[r * cols + c] = SAND;
                }
            }
            else if (material == WATER)
            {
                if (p_cells[(r + 1) * cols + c] != AIR && p_next_cells[r * cols + c - 1] != AIR && p_next_cells[r * cols + c - 1] != AIR)
                {
                    p_next_cells[r * cols + c] = WATER;
                }

                int left_cells = 0;
                Material left_cell = p_next_cells[r * cols + c - 1];
                while (left_cell == AIR)
                {
                    left_cells++;
                    left_cell = p_next_cells[r * cols + c - 1 - left_cells];
                }
                int right_cells = 0;
                Material right_cell = p_next_cells[r * cols + c + 1];
                while (right_cell == AIR)
                {
                    right_cells++;
                    right_cell = p_next_cells[r * cols + c + 1 + right_cells];
                }

                if (left_cells > right_cells)
                {
                    p_cells[r * cols + c] = AIR;
                    p_next_cells[r * cols + c - 1] = WATER;
                }
                else if (right_cells > left_cells)
                {
                    p_cells[r * cols + c] = AIR;
                    p_next_cells[r * cols + c + 1] = WATER;
                }
                else
                {
                    p_cells[r * cols + c] = AIR;
                    p_next_cells[r * cols + c + (((rand() >> 8) & 1) ? 1 : -1)] = WATER;
                }
            }
        }
    }
}

Uint32 get_material_colour(Material material)
{
    switch (material)
    {
    case (BOUNDARY):
        return 0xFFFFFF;
    case (WALL):
        return 0xFFFFFF;
    case (SAND):
        return 0xC80A0A;
    case (WATER):
        return 0x0000AA;
    default:
        return 0;
    }
}

void draw_cells(SDL_Surface *p_surface, Material *cells, int rows, int cols)
{
    SDL_Rect cell;
    cell.w = CELL_SIZE;
    cell.h = CELL_SIZE;

    Uint32 cell_colour = 0;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            if (cells[r * cols + c] != AIR)
            {
                cell.x = c * CELL_SIZE;
                cell.y = r * CELL_SIZE;
                cell_colour = get_material_colour(cells[r * cols + c]);
                SDL_FillRect(p_surface, &cell, cell_colour);
            }
        }
    }
}

int main()
{
    int rows = HEIGHT / CELL_SIZE + 1;
    int cols = WIDTH / CELL_SIZE;

    Material *p_cells;
    Material *p_next_cells;
    Material *tmp = NULL;

    p_cells = malloc(rows * cols * sizeof(Material));
    p_next_cells = malloc(rows * cols * sizeof(Material));

    memset(p_cells, AIR, rows * cols * sizeof(Material));
    memset(p_next_cells, AIR, rows * cols * sizeof(Material));

    for (int c = 0; c < cols; c++)
    {
        p_cells[(rows - 1) * cols + c] = BOUNDARY;
        p_next_cells[(rows - 1) * cols + c] = BOUNDARY;
    }

    srand(time(NULL));

    SDL_Window *p_window = SDL_CreateWindow("Falling Sand", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Surface *p_surface = SDL_GetWindowSurface(p_window);

    Uint32 black = SDL_MapRGB(p_surface->format, 0, 0, 0);
    // Uint32 claret = SDL_MapRGB(p_surface->format, 200, 10, 10);

    Material material = SAND;
    int radius = 25;
    int placing = 0;
    int destroying = 0;
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
                    placing = 1;
                }
                else if (button == 3)
                {
                    destroying = 1;
                }
            }
            if (event.type == SDL_MOUSEBUTTONUP)
            {
                button = event.button.button;
                if (button == 1)
                {
                    placing = 0;
                }
                else if (button == 3)
                {
                    destroying = 0;
                }
            }
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_r)
                {
                    memset(p_cells, AIR, rows * cols * sizeof(Material));
                    memset(p_next_cells, AIR, rows * cols * sizeof(Material));
                }
                if (event.key.keysym.sym == SDLK_s)
                {
                    material = SAND;
                    radius = 25;
                }
                else if (event.key.keysym.sym == SDLK_l)
                {
                    material = WATER;
                    radius = 10;
                }
                else if (event.key.keysym.sym == SDLK_b)
                {
                    material = WALL;
                    radius = 1;
                }
            }
        }
        SDL_FillRect(p_surface, NULL, black);

        if (placing)
        {
            modify_cells(radius, mouse_x, mouse_y, p_cells, rows, cols, material);
        }
        if (destroying)
        {
            modify_cells(25, mouse_x, mouse_y, p_cells, rows, cols, AIR);
        }

        update_cells(p_cells, p_next_cells, rows, cols);
        tmp = p_cells;
        p_cells = p_next_cells;
        p_next_cells = tmp;

        draw_cells(p_surface, p_cells, rows, cols);

        SDL_UpdateWindowSurface(p_window);
        SDL_Delay(8);
    }

    return 0;
}
