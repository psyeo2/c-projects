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

int get_word_length(char *word)
{
    int word_len = 0;
    while (word[word_len] != '\0')
        word_len++;
    return word_len;
}

int is_letter(char ch)
{
    if ((ch > 64 && ch < 91) || (ch > 96 && ch < 123))
        return 1;
    else
        return 0;
}

int get_ovp_idx(char *word)
{
    // printf("no seg\n");
    // return 0;
    int word_len = get_word_length(word);
    int front_punc = 0;

    while (word_len > 0 && !is_letter(word[word_len - 1]))
        word_len--;

    for (int i = 0; i < word_len; i++)
    {
        if (!is_letter(word[i]))
            front_punc++;
        else
            break;
    }

    word_len -= front_punc;

    if (word_len < 4)
        return word_len ? word_len / 2 + front_punc : front_punc;
    else if (word_len < 9)
        return word_len / 2 - 1 + front_punc;
    else
        return ((word_len * 2) / 7) ? (word_len * 2) / 7 + front_punc : 1 + front_punc;
}

void split_word(TTF_Font *font, SDL_Surface **left_surface, SDL_Surface **ovp_surface, SDL_Surface **right_surface, char *word, int ovp_idx)
{
    int word_len = get_word_length(word);
    char *left_chars = malloc(ovp_idx * sizeof(char) + 1);
    char *ovp_char = malloc(sizeof(char) + 1);
    char *right_chars = malloc(word_len - 1 - ovp_idx + 1);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color red = {255, 0, 0, 255};

    for (int i = 0; i < ovp_idx; i++)
        left_chars[i] = word[i];
    left_chars[ovp_idx] = '\0';
    ovp_char[0] = word[ovp_idx];
    ovp_char[1] = '\0';
    for (int i = ovp_idx + 1; i < word_len; i++)
        right_chars[i - (ovp_idx + 1)] = word[i];
    right_chars[word_len - ovp_idx - 1] = '\0';

    if (left_chars[0] != '\0')
        *left_surface = TTF_RenderText_Blended(font, left_chars, white);
    *ovp_surface = TTF_RenderText_Blended(font, ovp_char, red);
    if (right_chars[0] != '\0')
        *right_surface = TTF_RenderText_Blended(font, right_chars, white);

    free(left_chars);
    free(ovp_char);
    free(right_chars);
}

int print_word(SDL_Surface *p_surface, TTF_Font *font, int wpm, char *word, int i, int advance, double char_centre)
{
    int base = (60 * 1000) / wpm;
    int ovp_idx = get_ovp_idx(word);

    SDL_Surface *left_surface = NULL;
    SDL_Surface *ovp_surface = NULL;
    SDL_Surface *right_surface = NULL;
    SDL_Rect dst_left, dst_ovp, dst_right;

    split_word(font, &left_surface, &ovp_surface, &right_surface, word, ovp_idx);

    // printf("Word: '%s', word length: %d, ovp_idx: %d\n", word, get_word_length(word), ovp_idx);

    int left_w = left_surface ? left_surface->w : 0;
    int ovp_w = ovp_surface ? ovp_surface->w : 0;
    if (left_surface)
    {
        dst_left = (SDL_Rect){(int)(WIDTH / 2 - (ovp_idx * advance + char_centre)), HEIGHT / 2 - left_surface->h, left_surface->w, left_surface->h};
        SDL_BlitSurface(left_surface, NULL, p_surface, &dst_left);
        SDL_FreeSurface(left_surface);
    }
    if (ovp_surface)
    {
        dst_ovp = (SDL_Rect){(int)(WIDTH / 2 - (ovp_idx * advance + char_centre)) + left_w, HEIGHT / 2 - ovp_surface->h, ovp_surface->w, ovp_surface->h};
        SDL_BlitSurface(ovp_surface, NULL, p_surface, &dst_ovp);
        SDL_FreeSurface(ovp_surface);
    }
    if (right_surface)
    {
        dst_right = (SDL_Rect){(int)(WIDTH / 2 - (ovp_idx * advance + char_centre)) + left_w + ovp_w, HEIGHT / 2 - right_surface->h, ovp_w, right_surface->h};
        SDL_BlitSurface(right_surface, NULL, p_surface, &dst_right);
        SDL_FreeSurface(right_surface);
    }

    char last_char = 0;
    if (i > 2)
        last_char = word[i - 2];
    switch (last_char)
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
    TTF_Font *font = TTF_OpenFont("font.ttf", 32);
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
    SDL_Rect y_axis = (SDL_Rect){WIDTH / 2, 0, 2, HEIGHT};

    int render;

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
                    rewind(file);
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

        render = 0;
        if ((ch = fgetc(file)) != EOF)
        {
            if (i == 0 && (ch == ' ' || ch == '\n' || ch == '\r'))
                continue;
            if (ch == ' ' || ch == '\n' || ch == '\r')
            {
                    if (i < (int)sizeof(word) - 1)
                        word[i++] = '\0';
                    render = 1;
            }
            else if (i < (int)sizeof(word) - 1)
                word[i++] = ch;
        }
        else if (i > 0)
        {
            if (i < (int)sizeof(word) - 1)
                word[i++] = '\0';

            render = 1;
        }

        if (render)
        {
            // text_surface = TTF_RenderText_Blended(font, word, white);
            delay = print_word(p_surface, font, wpm, word, i, advance, char_centre);

            i = 0;

            SDL_UpdateWindowSurface(p_window);
            SDL_Delay(delay);
        }
    }

    fclose(file);
    TTF_Quit();
    return 0;
}
