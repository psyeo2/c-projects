#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#define WIDTH 900
#define HEIGHT 600
#define CELL_SIZE 10

int get_cell_status(int rows, int cols, int *p_grid, int r, int c)
{
    int status = 0;
    int neighbours =
    p_grid[(r-1)*cols + c-1] + p_grid[(r-1)*cols + c] + p_grid[(r-1)*cols + c+1] +
    p_grid[r*cols + c-1]     +                             p_grid[r*cols + c+1] +
    p_grid[(r+1)*cols + c-1] + p_grid[(r+1)*cols + c] + p_grid[(r+1)*cols + c+1];

    switch(neighbours)
    {
        case 0:
        case 1:
            status = 0;
            break;
        case 2:
            status = p_grid[r*cols + c];
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

    for(int r=1; r<rows-1; r++)
    {
        for(int c=1; c<cols-1; c++)
        {
            p_next_grid[r*cols + c] = get_cell_status(rows, cols, p_prev_grid, r, c);
        }
    }
}

void display_grid(SDL_Surface *pSurface, Uint32 colour, int rows, int cols, int *p_grid)
{
    for(int r=1; r<rows-1; r++)
    {
        for(int c=1; c<cols-1; c++)
        {
            if(p_grid[r*cols + c])
            {
                SDL_Rect cell_rect = (SDL_Rect)
                {
                    (c - 1)*CELL_SIZE,
                    (r - 1)*CELL_SIZE,
                    CELL_SIZE,
                    CELL_SIZE
                };
                SDL_FillRect(pSurface, &cell_rect, colour);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("Usage: ./game_of_life <updates-per-second>\n");
        return 1;
    }
    long fps = atol(argv[1]);
    if(!fps)
    {
        printf("Usage: ./game_of_life <updates-per-second>\n");
        return 1;
    }

    int frame_delay = (int)(1000/fps);

    SDL_Window *pWindow = SDL_CreateWindow(
        "Conway's Game of Life",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH,
        HEIGHT,
        0
    );

    SDL_Surface *pSurface = SDL_GetWindowSurface(pWindow);

    srand(time(NULL));
    
    Uint32 black = SDL_MapRGB(pSurface->format, 0, 0, 0);
    Uint32 green = SDL_MapRGB(pSurface->format, 0, 255, 0);

    int cols = WIDTH / CELL_SIZE + 2;
    int rows = HEIGHT / CELL_SIZE + 2;

    int prev_grid[rows][cols];
    int next_grid[rows][cols];
    memset(prev_grid, 0, sizeof(prev_grid));
    memset(next_grid, 0, sizeof(next_grid));

    double random;
    for(int r=0; r<rows; r++)
    {
        for(int c=0; c<cols; c++)
        {
            random = (double)rand()/RAND_MAX;
            if (random > 0.8)
                prev_grid[r][c] = 1;
        }
    }

    int *prev = &prev_grid[0][0];
    int *next = &next_grid[0][0];
    int *tmp;

    SDL_Event event;
    int app_running = 1;

    while(app_running)
    {
	    while(SDL_PollEvent(&event))
	    {
            if(event.type == SDL_QUIT)
	        {
		        app_running = 0;
	        }
	    }

        update_grid(rows, cols, prev, next);
        tmp = prev;
        prev = next;
        next = tmp;

        SDL_FillRect(pSurface, NULL, black);
        display_grid(pSurface, green, rows, cols, prev);

	    SDL_UpdateWindowSurface(pWindow);
	    SDL_Delay(frame_delay);
    }

    return 0;
}
