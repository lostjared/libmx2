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
    class mxUtil {
    public:
    #ifdef FOR_WASM
        std::string path = "/assets";
    #else
        std::string path = "assets";
    #endif
        std::string getFilePath(const std::string &filename);
        void printText(SDL_Renderer *renderer,TTF_Font *font, int x, int y, const std::string &text, SDL_Color col);
        SDL_Texture *loadTexture(SDL_Renderer *renderer, const std::string &filename);
        SDL_Texture *loadTexture(SDL_Renderer *renderer, const std::string &filename, int &w, int &h, bool color, SDL_Color key);
        SDL_Surface *loadSurface(const std::string &name);
        TTF_Font *loadFont(const std::string &filename, int size);
        void initJoystick();
        void closeJoystick();
        std::vector<SDL_Joystick *> stick;
    };
    std::vector<char> readFile(const std::string &filename);
    std::string decompressString(void *data, uLong size_);
    std::unique_ptr<char[]> compressString(const std::string &text, uLong &len);
    float generateRandomFloat(float min, float max);
    int generateRandomInt(int min, int max);
}

#endif