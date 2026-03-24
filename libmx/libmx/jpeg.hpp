/**
 * @file jpeg.hpp
 * @brief JPEG image loading and saving utilities (requires WITH_JPEG build flag).
 */
#ifndef __JPEG_H__
#define __JPEG_H__
#ifdef __EMSCRIPTEN__
#include "config.hpp"
#else
#include "config.h"
#endif
#ifdef WITH_JPEG
#include "SDL.h"

/** @namespace jpeg
 *  @brief Utilities for loading and saving JPEG images. */
namespace jpeg {

    /**
     * @brief Load a JPEG file into an SDL_Surface.
     * @param src Path to the JPEG file.
     * @return Newly allocated SDL_Surface, or nullptr on failure.
     */
    SDL_Surface *LoadJPEG(const char *src);

    /**
     * @brief Save an SDL_Texture to a JPEG file.
     * @param renderer SDL renderer used to read back pixel data.
     * @param texture  Source texture.
     * @param filename Destination file path.
     * @param quality  JPEG quality (0–100, default 90).
     * @return @c true on success, @c false on failure.
     */
    bool SaveJPEG(SDL_Renderer *renderer, SDL_Texture *texture, const char *filename, int quality = 90);
} // namespace jpeg
#endif
#endif