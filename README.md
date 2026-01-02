# C Projects

This is a small repo of personal pet projects written in C, with NO ChatGPT.

## Pre-Reqs

Most, if not all, of these projects require SDL. 

To use SDL:
```bash
sudo apt-get install libsdl2-dev
```

Then append the following to `gcc` compilation:
```bash
`sdl2-config --cflags --libs`

# e.g.:
gcc -o myprogram myprogram.c `sdl2-config --cflags --libs`
```

Some projects use `SDL_TTF` which can be installed as follows:
```bash
sudo apt-get install libsdl2-ttf-dev
```

And requires the `-lSDL2_ttf` flag to be appended to compilations, e.g.:
```bash
gcc -Wall -Wextra -o myprogram myprogram.c `sdl2-config --cflags --libs` -lSDL2_ttf
```