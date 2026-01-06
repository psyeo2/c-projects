#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WIDTH 900
#define HEIGHT 600

typedef enum
{
    INIT,
    RUNNING,
    PAUSED,
    QUIT
} State;

// int get_word_length(char *word)
// {
//     int word_len = 0;
//     while (word[word_len] != '\0')
//         word_len++;

//     return word_len;
// }

int get_ovp_idx(char *word)
{
    int word_len = 0;
    while (word[word_len] != '\0')
        word_len++;

    if (word_len < 5)
        return word_len ? word_len / 2 : 1;
    else if (word_len < 9)
        return word_len / 2 - 1;
    else
        return (word_len / 3) ? word_len / 3 : 1;
}

int print_word(SDL_Surface *p_surface, SDL_Surface *text_surface, int wpm, char *word, int i, int advance, double char_centre)
{
    int base = (60 * 1000) / wpm;
    int ovp_idx = get_ovp_idx(word);

    SDL_Rect dst = {(int)(WIDTH / 2 - (ovp_idx * advance + char_centre)), HEIGHT / 2 - text_surface->h, text_surface->w, text_surface->h};

    SDL_BlitSurface(text_surface, NULL, p_surface, &dst);

    switch (word[i - 2])
    {
    case (','):
    case (';'):
    case ('"'):
    case ('\''):
    case (')'):
        return (int)(base * 1.3);
        break;
    case ('.'):
    case ('?'):
    case ('!'):
    case (':'):
    case ('-'):
        return (int)(base * 1.8);
        break;
    default:
        break;
    }
    return base;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <file-name.txt>\n", argv[0]);
        return 1;
    }
    if (TTF_Init() != 0)
    {
        fprintf(stderr, "TTF Error\n");
        return 1;
    }
    TTF_Font *font = TTF_OpenFont("font.ttf", 22);
    if (!font)
    {
        fprintf(stderr, "Load font error: %s\n", TTF_GetError());
        return 1;
    }

    const char *filename = argv[1];
    FILE *file;
    file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "File load error. Is %s the correct filename?\n", filename);
        return 1;
    }
    char word[100];
    int ch;
    int i = 0;

    int delay;

    int wpm = 300;

    int minx, maxx, advance;

    TTF_GlyphMetrics32(font, 'A', &minx, &maxx, NULL, NULL, &advance);
    double char_centre = (maxx - minx) / 2.0;

    SDL_Window *p_window = SDL_CreateWindow("Rapid Serial Visual Presentation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Surface *p_surface = SDL_GetWindowSurface(p_window);
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *text_surface;
    SDL_Rect y_axis = (SDL_Rect){WIDTH / 2, 0, 2, HEIGHT};

    SDL_Event event;

    Uint32 black = SDL_MapRGB(p_surface->format, 0, 0, 0);
    Uint32 grey = SDL_MapRGB(p_surface->format, 50, 50, 50);
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
                switch (event.key.keysym.sym)
                {
                case SDLK_r:
                    fclose(file);
                    file = fopen(filename, "r");
                    if (!file)
                    {
                        fprintf(stderr, "File load error. Is %s the correct filename?\n", filename);
                        return 1;
                    }
                    break;
                case SDLK_1:
                    wpm = 100;
                    break;
                case SDLK_2:
                    wpm = 200;
                    break;
                case SDLK_3:
                    wpm = 300;
                    break;
                case SDLK_4:
                    wpm = 400;
                    break;
                case SDLK_5:
                    wpm = 500;
                    break;
                case SDLK_6:
                    wpm = 600;
                    break;
                case SDLK_7:
                    wpm = 700;
                    break;
                case SDLK_8:
                    wpm = 800;
                    break;
                case SDLK_9:
                    wpm = 900;
                    break;
                }
            }
        }

        SDL_FillRect(p_surface, NULL, black);
        SDL_FillRect(p_surface, &y_axis, grey);

        if ((ch = fgetc(file)) != EOF)
        {
            if (ch == ' ' || ch == '\n' || ch == '\r')
            {
                if (i < (int)sizeof(word) - 1)
                    word[i++] = '\0';

                text_surface = TTF_RenderText_Blended(font, word, white);
                delay = print_word(p_surface, text_surface, wpm, word, i, advance, char_centre);
                i = 0;
                SDL_FreeSurface(text_surface);

                SDL_UpdateWindowSurface(p_window);
                SDL_Delay(delay);
            }
            else if (i < (int)sizeof(word) - 1)
                word[i++] = ch;
        }
        else if (i > 0)
        {
            if (i < (int)sizeof(word) - 1)
                word[i++] = '\0';

            text_surface = TTF_RenderText_Blended(font, word, white);
            delay = print_word(p_surface, text_surface, wpm, word, i, advance, char_centre);
            SDL_FreeSurface(text_surface);

            SDL_UpdateWindowSurface(p_window);
            SDL_Delay(delay);
        }
    }

    fclose(file);
    return 0;
}
