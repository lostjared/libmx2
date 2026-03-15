/**
 * @file texture.hpp
 * @brief SDL_Texture RAII wrapper with loading, saving, and flip utilities.
 */
#ifndef _TEXTURE_H___
#define _TEXTURE_H___

#include"SDL.h"
#include<string>
#include<iostream>
#include<vector>
#include"tee_stream.hpp"
#include<optional>
#include"wrapper.hpp"

namespace mx {

    class mxWindow;

/**
 * @class Texture
 * @brief RAII wrapper around an SDL_Texture.
 *
 * Manages the lifetime of an SDL_Texture and provides helpers for loading
 * from image files, creating blank textures, saving to BMP, and an in-place
 * vertical flip of an SDL_Surface.  Only move semantics are supported;
 * copying is disabled to prevent double-free.
 */
    class Texture {
    public:
        /** @brief Default constructor — no texture loaded. */
        Texture() = default;
        Texture(const Texture &tex) = delete;

        /** @brief Move constructor — transfers ownership of the SDL_Texture. */
        Texture(Texture &&tex);
        Texture &operator=(const Texture &tex) = delete;

        /** @brief Move-assign — transfers ownership of the SDL_Texture. */
        Texture &operator=(Texture &&tex);

        /** @brief Destructor — destroys the SDL_Texture if held. */
        ~Texture();

        /**
         * @brief Create an empty (blank) texture of the given dimensions.
         * @param window Owning window supplying the renderer.
         * @param width  Texture width in pixels.
         * @param height Texture height in pixels.
         */
        void createTexture(mxWindow *window, int width, int height);

        /**
         * @brief Load a texture from an image file.
         * @param window   Owning window supplying the renderer.
         * @param filename Path to the image file (PNG, BMP, etc.).
         */
        void loadTexture(mxWindow *window, const std::string &filename);

        /**
         * @brief Load a texture with optional colour-key transparency.
         * @param window   Owning window.
         * @param filename Image file path.
         * @param w        Output: loaded image width.
         * @param h        Output: loaded image height.
         * @param color    If @c true, apply the colour key.
         * @param key      Colour to treat as transparent.
         */
        void loadTexture(mxWindow *window, const std::string &filename, int &w, int &h, bool color, SDL_Color key);

        /**
         * @brief Save the texture to a file.
         * @param window   Owning window supplying the renderer.
         * @param filename Destination file path (BMP format).
         * @return @c true on success.
         */
        bool saveTexture(mxWindow *window, const std::string &filename);

        /**
         * @brief Return the SDL_Texture as an optional.
         * @return std::optional with the texture, or std::nullopt if not loaded.
         */
        std::optional<SDL_Texture *> handle() const { 
            if(texture)
                return texture; 
            return std::nullopt;
        }

        /**
         * @brief Unwrap the SDL_Texture pointer, throwing on null.
         * @return Raw SDL_Texture pointer.
         * @throws mx::Exception if texture is not loaded.
         */
        SDL_Texture *unwrap() const {
            if(texture) {
                return texture;
            }
            throw Exception("unwrap: Invalid texture");
            return 0;
        }

        /**
         * @brief Return a Wrapper around the SDL_Texture handle.
         * @return Wrapper<SDL_Texture*> holding the pointer or nullopt.
         */
        Wrapper<SDL_Texture *> wrapper() const {
            if(texture)
                return texture;
            
            return std::nullopt;
        }

        /**
         * @brief Flip an SDL_Surface vertically (in-place).
         * @param surface Surface to flip.
         * @return The same surface pointer after flipping.
         */
        static SDL_Surface* flipSurface(SDL_Surface* surface) {
            if (SDL_LockSurface(surface) != 0) {
                return surface;
            }
            Uint8* pixels = (Uint8*)surface->pixels;
            int pitch = surface->pitch;
            std::vector<Uint8> tempRow(pitch);
            for (int y = 0; y < surface->h / 2; ++y) {
                Uint8* row1 = pixels + y * pitch;
                Uint8* row2 = pixels + (surface->h - y - 1) * pitch;
                memcpy(tempRow.data(), row1, pitch);
                memcpy(row1, row2, pitch);
                memcpy(row2, tempRow.data(), pitch);
            }
            SDL_UnlockSurface(surface);
            return surface;
        }

        /** @return Width of the loaded texture in pixels. */
        int width() const { return width_; }
        /** @return Height of the loaded texture in pixels. */
        int height() const { return height_; }
    private:
        SDL_Texture *texture = nullptr; ///< Managed SDL texture handle.
        int width_ = 0;                 ///< Cached texture width.
        int height_ = 0;                ///< Cached texture height.

        int imageType(const std::string &type);
        bool saveBMP(SDL_Texture *texture, SDL_Renderer *renderer, const std::string &name);
    };

}

#endif