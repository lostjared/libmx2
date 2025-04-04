#ifndef __LOADPNG_H__
#define __LOADPNG_H__
#include"SDL.h"
namespace png {
    SDL_Surface *LoadPNG(const char* file);
    bool SavePNG(SDL_Texture* texture, SDL_Renderer* renderer, const char* filename);
    bool SavePNG_RGBA(const char *filename, void *buffer, int w, int h);
}
#endif