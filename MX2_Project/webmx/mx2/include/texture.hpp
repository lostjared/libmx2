#ifndef _TEXTURE_H___
#define _TEXTURE_H___

#include"SDL.h"
#include<string>
#include<iostream>
#include"tee_stream.hpp"
#include<optional>
#include"wrapper.hpp"

namespace mx {

    class mxWindow;

    class Texture {
    public:
        Texture() = default;
        Texture(const Texture &tex) = delete;
        Texture(Texture &&tex);
        Texture &operator=(const Texture &tex) = delete;
        Texture &operator=(Texture &&tex);
        ~Texture();
        void createTexture(mxWindow *window, int width, int height);
        void loadTexture(mxWindow *window, const std::string &filename);
        void loadTexture(mxWindow *window, const std::string &filename, int &w, int &h, bool color, SDL_Color key);
        bool saveTexture(mxWindow *window, const std::string &filename);

        std::optional<SDL_Texture *> handle() const { 
            if(texture)
                return texture; 
            return std::nullopt;
        }
        SDL_Texture *unwrap() const {
            if(texture) {
                return texture;
            }
            throw Exception("unwrap: Invalid texture");
            return 0;
        }

        Wrapper<SDL_Texture *> wrapper() const {
            if(texture)
                return texture;
            
            return std::nullopt;
        }

        static SDL_Surface* flipSurface(SDL_Surface* surface) {
            SDL_LockSurface(surface);
            Uint8* pixels = (Uint8*)surface->pixels;
            int pitch = surface->pitch;
            Uint8* tempRow = new Uint8[pitch];
            for (int y = 0; y < surface->h / 2; ++y) {
                Uint8* row1 = pixels + y * pitch;
                Uint8* row2 = pixels + (surface->h - y - 1) * pitch;
                memcpy(tempRow, row1, pitch);
                memcpy(row1, row2, pitch);
                memcpy(row2, tempRow, pitch);
            }
            
            delete[] tempRow;
            SDL_UnlockSurface(surface);
            return surface;
        }

        int width() const { return width_; }
        int height() const { return height_; }
    private:
        SDL_Texture *texture = nullptr;
        int width_ = 0, height_ = 0;

        int imageType(const std::string &type);
        bool saveBMP(SDL_Texture *texture,SDL_Renderer *renderer,const std::string &name);
    };


}

#endif