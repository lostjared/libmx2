#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include<vector>
#include<string>
#include<iostream>
#include "SDL.h"
#include "SDL_ttf.h"


namespace mx {

    class mxUtil {
    public:
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

}

#endif