#include <stdio.h>
#include <SDL2/SDL.h>
#include "tinyexpr.h"

#define WIDTH 900
#define HEIGHT 600

typedef enum
{
    INIT,
    RUNNING,
    PAUSED,
    QUIT
} State;

double pixel_x_to_graph_x(int x, double scale)
{
    return ((double)x - (WIDTH / 2)) * scale;
}
double pixel_y_to_graph_y(int y, double scale)
{
    return -((double)y - (HEIGHT / 2)) * scale;
}

int graph_x_to_pixel_x(double x, double scale)
{
    return (int)((WIDTH / 2 + x / scale));
}
int graph_y_to_pixel_y(double y, double scale)
{
    return (int)(((y / scale) / -1) + HEIGHT / 2);
}

void plot_axes(SDL_Surface *p_surface, Uint32 colour)
{
    int axis_thickness = 3;
    SDL_Rect x_axis = (SDL_Rect){0, HEIGHT / 2 - axis_thickness / 2, WIDTH, axis_thickness};
    SDL_Rect y_axis = (SDL_Rect){WIDTH / 2 - axis_thickness / 2, 0, axis_thickness, HEIGHT};

    SDL_FillRect(p_surface, &x_axis, colour);
    SDL_FillRect(p_surface, &y_axis, colour);
}

int y_for_all_x(char *expression, double *graph_ys, double scale)
{
    double x;
    te_variable vars[] = {{"x", &x, TE_VARIABLE, NULL}};
    int tinyexpr_error;

    te_expr *n = te_compile(expression, vars, 1, &tinyexpr_error);

    if (n)
    {
        for (int i = 0; i < WIDTH; i++)
        {
            x = pixel_x_to_graph_x(i, scale);
            const double r = te_eval(n);
            graph_ys[i] = r;
        }
        te_free(n);
        return 0;
    }
    else
    {
        printf("\t%*s^\nError near here", tinyexpr_error - 1, "");
        printf("\n");
        return -1;
    }
}

void draw_line(SDL_Surface *p_surface, double *graph_ys, Uint32 colour, double scale)
{
    for (int i = 0; i < WIDTH; i++)
    {
        int x1 = i;
        int y1 = graph_y_to_pixel_y(graph_ys[i], scale);
        int x2 = i + 1;
        int y2 = graph_y_to_pixel_y(graph_ys[i + 1], scale);
    }
}

void plot_function(SDL_Surface *p_surface, double *graph_ys, Uint32 colour, double scale)
{
    SDL_Rect pixel;
    pixel.h = 1;
    pixel.w = 1;

    for (int i = 0; i < WIDTH; i++)
    {
        // pixel.x = graph_x_to_pixel_x(graph_xs[i], scale);
        pixel.x = i;
        pixel.y = graph_y_to_pixel_y(graph_ys[i], scale);
        SDL_FillRect(p_surface, &pixel, colour);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: ./function_plotter <function>\n");
        return 1;
    }
    char *expression = argv[1];

    double scale = 0.05;

    double graph_ys[WIDTH];

    for (int i = 0; i < WIDTH; i++)
    {
        graph_ys[i] = 0.0;
    }

    printf("Evaluating:\n\t%s\n", expression);
    if (y_for_all_x(expression, &graph_ys[0], scale) == -1)
    {
        return 1;
    }

    SDL_Window *p_window = SDL_CreateWindow("Function Plotter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Surface *p_surface = SDL_GetWindowSurface(p_window);

    Uint32 black = SDL_MapRGB(p_surface->format, 0, 0, 0);
    Uint32 white = SDL_MapRGB(p_surface->format, 255, 255, 255);
    Uint32 grey = SDL_MapRGB(p_surface->format, 45, 45, 45);

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
            if (event.type == SDL_MOUSEWHEEL)
            {
                // printf("wheel.y: %d, wheel.x: %d", event.wheel.y, event.wheel.x);
                if (event.wheel.y < 0)
                {
                    scale /= 0.8;
                }
                else if (event.wheel.y > 0)
                {
                    scale *= 0.8;
                }
                y_for_all_x(expression, &graph_ys[0], scale);
                // printf("Scale = %f\n", scale);
            }
        }
        SDL_FillRect(p_surface, NULL, black);

        plot_axes(p_surface, grey);
        plot_function(p_surface, &graph_ys[0], white, scale);

        SDL_UpdateWindowSurface(p_window);
        SDL_Delay(16);
    }

    return 0;
}
