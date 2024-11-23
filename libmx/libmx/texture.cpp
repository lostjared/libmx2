#include"mx.hpp"
#include"texture.hpp"
#include"loadpng.hpp"
#ifdef WITH_JPEG
#include"jpeg.hpp"
#endif
namespace mx {
    
   // Texture::Texture(const Texture &tex) : texture{tex.texture}, width_{tex.width_}, height_{tex.height_} {}
 
    Texture::Texture(Texture &&tex) : texture{tex.texture}, width_{tex.width_}, height_{tex.height_} {
        tex.texture = nullptr;
    }
 
/*
     Texture &Texture::operator=(const Texture &tex) {
        texture = tex.texture;
        width_ = tex.width_;
        height_ = tex.height_;
        return *this;
    }
*/
    Texture &Texture::operator=(Texture &&tex) {
        texture = tex.texture;
        tex.texture = nullptr;
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

        if(texture != nullptr) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }

        SDL_Surface *surface = nullptr;
        int type = imageType(filename);
        if(type == 1) {
            #ifdef WITH_JPEG
            surface = jpeg::LoadJPEG(filename.c_str());
            #else
            mx::system_err << "mx: JPEG for file not suported: " << filename << "\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
            #endif
        } else if(type == 2) {
            surface = png::LoadPNG(filename.c_str());
        } else if(type == 3) {
            surface = SDL_LoadBMP(filename.c_str());
        } else {
            mx::system_err << "mx: Image format not suported for: " << filename << "\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
        }
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
        if(surface)
            SDL_FreeSurface(surface);

        texture = tex;
    }

    bool Texture::saveTexture(mxWindow *window, const std::string &filename) {
        int image_t = imageType(filename);
        switch(image_t) { 
            case 1:
#if WITH_JPEG
                return jpeg::SaveJPEG(window->renderer, texture, filename.c_str());
#else
                mx::system_err << "mx: No JPEG Support..\n";
                mx::system_err.flush();
                exit(EXIT_FAILURE);
#endif
            case 2:
                return png::SavePNG(texture, window->renderer, filename.c_str());
            case 3:
                return this->saveBMP(texture, window->renderer, filename);
            default:
                mx::system_err << "mx: Invalid image type..\n";
                mx::system_err.flush();
                exit(EXIT_FAILURE);
                return false;
        }
    }
 
    Texture::~Texture() {
        if(texture)
            SDL_DestroyTexture(texture);
    }

    int Texture::imageType(const std::string &filename) {
        std::string lwr;
        for(size_t i =  0; i < filename.length(); ++i) 
            lwr += tolower(filename[i]);
        if(lwr.find(".jpg") != std::string::npos)
            return 1;
        if(lwr.find(".png") != std::string::npos)
            return 2;
        if(lwr.find(".bmp") != std::string::npos)
            return 3;
        return 0;
    }

    bool Texture::saveBMP(SDL_Texture *texturex,SDL_Renderer *renderer, const std::string &filename) {
        int width = 0, height = 0;
        Uint32 format = 0;
        int access = 0;
        if (SDL_QueryTexture(texturex, &format, &access, &width, &height) != 0) {
            return false;
        }
        SDL_Texture *old = SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer, texturex);
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 24, SDL_PIXELFORMAT_RGB24);
        if (!surface) {
            return false;
        }
        if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGB24, surface->pixels, surface->pitch) != 0) {
            SDL_FreeSurface(surface);
            return false;
        }
        SDL_SaveBMP(surface, filename.c_str());
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(renderer, old);
        return true;
    }

}