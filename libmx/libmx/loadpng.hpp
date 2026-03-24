/**
 * @file loadpng.hpp
 * @brief PNG image loading and saving utilities via SDL.
 */
#ifndef __LOADPNG_H__
#define __LOADPNG_H__
#include "SDL.h"

/** @namespace png
 *  @brief Utilities for loading and saving PNG images. */
namespace png {

    /**
     * @brief Load a PNG file into an SDL_Surface.
     * @param file Path to the PNG file.
     * @return Newly allocated SDL_Surface, or nullptr on failure.
     */
    SDL_Surface *LoadPNG(const char *file);

    /**
     * @brief Save an SDL_Texture to a PNG file.
     * @param texture  Source texture.
     * @param renderer Renderer used to read back the texture pixels.
     * @param filename Destination file path.
     * @return @c true on success, @c false on failure.
     */
    bool SavePNG(SDL_Texture *texture, SDL_Renderer *renderer, const char *filename);

    /**
     * @brief Save a raw RGBA pixel buffer to a PNG file.
     * @param filename Destination file path.
     * @param buffer   Pointer to the raw RGBA pixel data.
     * @param w        Image width in pixels.
     * @param h        Image height in pixels.
     * @return @c true on success, @c false on failure.
     */
    bool SavePNG_RGBA(const char *filename, void *buffer, int w, int h);
} // namespace png
#endif