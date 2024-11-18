#include"mx.hpp"
#include"texture.hpp"
#include"loadpng.hpp"

namespace mx {
    
    Texture::Texture(const Texture &tex) : texture{tex.texture}, width_{tex.width_}, height_{tex.height_} {}
 
    Texture::Texture(Texture &&tex) : texture{tex.texture}, width_{tex.width_}, height_{tex.height_} {}
 
    Texture &Texture::operator=(const Texture &tex) {
        texture = tex.texture;
        width_ = tex.width_;
        height_ = tex.height_;
        return *this;
    }
 
    Texture &Texture::operator=(Texture &&tex) {
        texture = tex.texture;
        width_ = tex.width_;
        height_ = tex.height_;
        return *this;
    }

    void Texture::createTexture(mxWindow *window, int width, int height) {
        SDL_Texture *tex = SDL_CreateTexture(window->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
        if(!tex) {
            mx::system_err << "mx: Error creating texture: " << SDL_GetError() << "\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
        }
        texture = tex;
    }

    void Texture::loadTexture(mxWindow *window, const std::string &filename) {
        int w = 0, h = 0;
        return loadTexture(window, filename, w, h, false, {0,0,0,0});
    }
    
    void Texture::loadTexture(mxWindow *window, const std::string &filename, int &w, int &h, bool color, SDL_Color key) {
        SDL_Surface *surface = png::LoadPNG(filename.c_str());
        if(!surface) {
            mx::system_err << "mx: Error could not open file: " << filename << "\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
        }
        if(color)
            SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, key.r, key.g, key.b));

        w = surface->w;
        h = surface->h;
        width_ = w;
        height_ = h;

        SDL_Texture *tex = SDL_CreateTextureFromSurface(window->renderer, surface);
        if(!tex) {
            mx::system_err << "mx: Error creating texture from surface..\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
        }
        texture = tex;
    }
 
    Texture::~Texture() {
        if(texture)
            SDL_DestroyTexture(texture);
    }


}