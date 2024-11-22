#ifndef __JPEG_H__
#define __JPEG_H__
#ifdef WITH_JPEG
#include "SDL.h"
namespace jpeg {
SDL_Surface *LoadJPEG(const char *src);
}
#endif
#endif