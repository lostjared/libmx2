#include"loadpng.hpp"

namespace png {
    SDL_Surface* LoadPNG(const char* file) {
        return IMG_Load(file);
    }

    bool SavePNG_RGBA(const char *filename, void *buffer, int w, int h) {
        return true;
    }
}