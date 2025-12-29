#include <stdio.h>
#include <SDL2/SDL.h>

#define WIDTH 900
#define HEIGHT 600

int main()
{
    SDL_Window *pWindow = SDL_CreateWindow("Hello World", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Surface *pSurface = SDL_GetWindowSurface(pWindow);
    
    SDL_Rect rect = (SDL_Rect) {50,50,50,50};
    Uint32 colour = 0x00FF00;
    SDL_FillRect(pSurface, &rect, colour);

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
	SDL_UpdateWindowSurface(pWindow);
	SDL_Delay(16);
    }

    return 0;
}
