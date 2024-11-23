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
        Texture(const Texture &tex);
        Texture(Texture &&tex);
        Texture &operator=(const Texture &tex);
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
            mx::system_err << "mx: panic Invalid Texture.\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
            return 0;
        }

        Wrapper<SDL_Texture *> wrapper() const {
            if(texture)
                return texture;
            
            return std::nullopt;
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