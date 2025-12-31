#include <stdio.h>
#include <SDL2/SDL.h>

#define WIDTH 900
#define HEIGHT 600

typedef enum
{
    INIT,
    RUNNING,
    PAUSED,
    QUIT
} State;

int main()
{
    SDL_Window *p_window = SDL_CreateWindow("Hello World!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Surface *p_surface = SDL_GetWindowSurface(p_window);

    SDL_Event event;

    State state = RUNNING;
    while (state != QUIT)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT || (event.type = SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
            {
                state = QUIT;
            }
        }
        SDL_UpdateWindowSurface(p_window);
        SDL_Delay(16);
    }

    return 0;
}
