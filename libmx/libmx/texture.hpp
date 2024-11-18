#ifndef _TEXTURE_H___
#define _TEXTURE_H___

#include"SDL.h"
#include<string>
#include<iostream>
#include"tee_stream.hpp"

namespace mx {

    class mxWindow;

    class Texture {
    public:
        Texture() = default;
        Texture(const Texture &tex);
        Texture(Texture &&tex);
        Texture &operator=(const Texture &tex);
        Texture &operator=(Texture &&tex);
        ~Texture();
        void createTexture(mxWindow *window, int width, int height);
        void loadTexture(mxWindow *window, const std::string &filename);
        void loadTexture(mxWindow *window, const std::string &filename, int &w, int &h, bool color, SDL_Color key);
        SDL_Texture *handle() const { return texture; }
        int width() const { return width_; }
        int height() const { return height_; }
    private:
        SDL_Texture *texture = nullptr;
        int width_ = 0, height_ = 0;
    };


}

#endif