#ifndef __LOADPNG_H__
#define __LOADPNG_H__
#include"SDL_image.h"
namespace png {
    SDL_Surface *LoadPNG(const char* file);
}
#endif