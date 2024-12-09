#ifndef __JPEG_H__
#define __JPEG_H__
#ifdef __EMSCRIPTEN__
#include"config.hpp"
#else
#include"config.h"
#endif
#ifdef WITH_JPEG
#include "SDL.h"
namespace jpeg {
SDL_Surface *LoadJPEG(const char *src);
bool SaveJPEG(SDL_Renderer *, SDL_Texture *, const char *, int quality = 90);
}
#endif
#endif