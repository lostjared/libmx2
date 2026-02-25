/**
 * @file util.hpp
 * @brief General SDL utility helpers, file I/O, and compression utilities.
 *
 * Provides mxUtil for window-attached helpers (texture loading, font loading,
 * joystick management) plus free functions for file reading, string
 * compression/decompression (zlib), and random number generation.
 */
#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include<vector>
#include<string>
#include<iostream>
#include "SDL.h"
#include "SDL_ttf.h"
#include"tee_stream.hpp"
#include<optional>
#include<memory>
#include<zlib.h>
#include<vector>

namespace mx {

/**
 * @class mxUtil
 * @brief Utility bag for SDL-based applications.
 *
 * Holds the asset path and provides convenience wrappers for loading textures,
 * fonts, printing text, and managing SDL joystick handles.  Non-copyable and
 * non-movable by design — embed one instance per window.
 */
    class mxUtil {
    public:
        /** @brief Destructor — closes all open joystick handles. */
        ~mxUtil() { closeJoystick(); }

        mxUtil() = default;
        mxUtil(const mxUtil&) = delete;
        mxUtil& operator=(const mxUtil&) = delete;
        mxUtil(mxUtil&&) = delete;
        mxUtil& operator=(mxUtil&&) = delete;

    #ifdef FOR_WASM
        std::string path = "/assets"; ///< Root asset directory (Emscripten).
    #else
        std::string path = "assets";  ///< Root asset directory (native).
    #endif

        /**
         * @brief Prepend the asset path to a filename.
         * @param filename Relative filename.
         * @return Full path string.
         */
        std::string getFilePath(const std::string &filename);

        /**
         * @brief Render a text string onto an SDL_Renderer.
         * @param renderer Active renderer.
         * @param font     TTF font to use.
         * @param x        X position.
         * @param y        Y position.
         * @param text     String to draw.
         * @param col      Text colour.
         */
        void printText(SDL_Renderer *renderer, TTF_Font *font, int x, int y, const std::string &text, SDL_Color col);

        /**
         * @brief Load an SDL_Texture from a file.
         * @param renderer Active renderer.
         * @param filename Image file path (relative to asset root).
         * @return Loaded texture (caller owns it), or nullptr on failure.
         */
        SDL_Texture *loadTexture(SDL_Renderer *renderer, const std::string &filename);

        /**
         * @brief Load an SDL_Texture with colour-key transparency.
         * @param renderer Active renderer.
         * @param filename Image file path.
         * @param w        Output: image width.
         * @param h        Output: image height.
         * @param color    Apply colour key if @c true.
         * @param key      Colour to treat as transparent.
         * @return Loaded texture, or nullptr on failure.
         */
        SDL_Texture *loadTexture(SDL_Renderer *renderer, const std::string &filename, int &w, int &h, bool color, SDL_Color key);

        /**
         * @brief Load an image as an SDL_Surface.
         * @param name Image file path.
         * @return Newly allocated SDL_Surface, or nullptr on failure.
         */
        SDL_Surface *loadSurface(const std::string &name);

        /**
         * @brief Load a TTF font at the specified point size.
         * @param filename Font file path.
         * @param size     Point size.
         * @return Loaded TTF_Font handle (caller owns it), or nullptr on failure.
         */
        TTF_Font *loadFont(const std::string &filename, int size);

        /** @brief Initialise and open SDL joysticks. */
        void initJoystick();

        /** @brief Close all open SDL joystick handles. */
        void closeJoystick();

        std::vector<SDL_Joystick *> stick; ///< Open joystick handles.
    };

    /**
     * @brief Read an entire file into a byte vector.
     * @param filename Path to the file.
     * @return Vector of bytes.
     */
    std::vector<char> readFile(const std::string &filename);

    /**
     * @brief Read an entire text file into a string.
     * @param filename Path to the file.
     * @return File contents as a string.
     */
    std::string readFileToString(const std::string &filename);

    /**
     * @brief Decompress a zlib-compressed data block.
     * @param data  Pointer to compressed data.
     * @param size_ Size of compressed data in bytes.
     * @return Decompressed string.
     */
    std::string decompressString(void *data, uLong size_);

    /**
     * @brief Compress a string using zlib.
     * @param text Input string.
     * @param len  Output: length of the compressed data.
     * @return Unique pointer to the compressed byte array.
     */
    std::unique_ptr<char[]> compressString(const std::string &text, uLong &len);

    /**
     * @brief Generate a random float in the given range.
     * @param min Lower bound (inclusive).
     * @param max Upper bound (inclusive).
     * @return Random float in [min, max].
     */
    float generateRandomFloat(float min, float max);

    /**
     * @brief Generate a random integer in the given range.
     * @param min Lower bound (inclusive).
     * @param max Upper bound (inclusive).
     * @return Random integer in [min, max].
     */
    int generateRandomInt(int min, int max);
}

#endif