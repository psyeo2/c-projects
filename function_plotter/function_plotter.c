#include <stdio.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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

// void draw_line(SDL_Surface *p_surface, double *graph_ys, Uint32 colour, double scale)
// {
//     SDL_Rect pixel;
//     pixel.h = 1;
//     pixel.w = 1;

//     for (int i = 0; i < WIDTH; i++)
//     {
//         int x1 = i;
//         int y1 = graph_y_to_pixel_y(graph_ys[i], scale);
//         int x2 = i + 1;
//         int y2 = graph_y_to_pixel_y(graph_ys[i + 1], scale);

//         double m = (y2 - y1) / (x2 - x1);
//         double c = y1 - m * x1;

//         for (int y = y1; y < y2; y++)
//         {
//             pixel.y = y;
//             double given_x = (y - c) / m;
//             pixel.x = (int)given_x
//         }
//     }
// }

void draw_line(SDL_Surface *surface,
               int x0, int y0,
               int x1, int y1,
               Uint32 colour)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1)
    {
        if (x0 >= 0 && x0 < WIDTH && y0 >= 0 && y0 < HEIGHT)
        {
            SDL_Rect p = {x0, y0, 1, 1};
            SDL_FillRect(surface, &p, colour);
        }

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void plot_function(SDL_Surface *p_surface, double *graph_ys, Uint32 colour, double scale)
{
    for (int i = 1; i < WIDTH; i++)
    {
        if (!isfinite(graph_ys[i]) || !isfinite(graph_ys[i - 1]))
            continue;

        int y0 = graph_y_to_pixel_y(graph_ys[i - 1], scale);
        int y1 = graph_y_to_pixel_y(graph_ys[i], scale);

        if (abs(y1 - y0) > HEIGHT)
        {
            int edge_y;

            if (y1 > y0)
                edge_y = HEIGHT - 1; // going down
            else
                edge_y = 0; // going up

            draw_line(p_surface,
                      i - 1, y0,
                      i - 1, edge_y,
                      colour);

            continue;
        }

        draw_line(p_surface,
                  i - 1, y0,
                  i, y1,
                  colour);
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

    if (TTF_Init() != 0)
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

    char scale_text[100];
    sprintf(scale_text, "%.3f", 1.0 / scale);

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
                sprintf(scale_text, "%.3f", 1.0 / scale);
                // printf("Scale = %f\n", scale);
            }
        }
        SDL_FillRect(p_surface, NULL, black);

        plot_axes(p_surface, grey);
        plot_function(p_surface, &graph_ys[0], white, scale);

        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface *text_surface = TTF_RenderText_Blended(font, scale_text, white);
        SDL_Rect dst = {10, 10, text_surface->w, text_surface->h};
        SDL_BlitSurface(text_surface, NULL, p_surface, &dst);

        SDL_UpdateWindowSurface(p_window);
        SDL_Delay(16);
        SDL_FreeSurface(text_surface);
    }

    return 0;
}
