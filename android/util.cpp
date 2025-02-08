#include"util.hpp"
#include"loadpng.hpp"
#include<sstream>
#include<string>
#include"SDL.h"
#include"SDL_image.h"
namespace mx {

    std::string mxUtil::getFilePath(const std::string &filename) {
        SDL_Log("Loading file: %s\n", filename.c_str());
        return filename;
    }

    void mxUtil::printText(SDL_Renderer *renderer,TTF_Font *font,int x, int y, const std::string &text, SDL_Color col) {
        SDL_Surface *surf = TTF_RenderText_Blended(font,text.c_str(), col);
        if(!surf) {
            //std::cerr << "mx: Error rendering text...\n";
            return;
        }
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        if(!tex) {
            //std::cerr << "mx: Error creating texture..\n";
            SDL_FreeSurface(surf);
            return;
        }
        SDL_Rect rc = {x,y,surf->w,surf->h};
        SDL_RenderCopy(renderer, tex, nullptr, &rc);
        SDL_FreeSurface(surf);
        SDL_DestroyTexture(tex);
    }

    SDL_Texture *mxUtil::loadTexture(SDL_Renderer *renderer, const std::string &filename) {
        int w = 0, h = 0;
        return loadTexture(renderer, filename, w, h, false, {0,0,0,0});
    }
    
    SDL_Texture *mxUtil::loadTexture(SDL_Renderer *renderer, const std::string &filename, int &w, int &h, bool color, SDL_Color key) {
        SDL_Surface *surface = IMG_Load(getFilePath(filename).c_str());
        if(!surface) {
            SDL_Log("mx: Error could not open file: %s", getFilePath(filename).c_str());
            exit(EXIT_FAILURE);
        }
        if(color)
            SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, key.r, key.g, key.b));

        w = surface->w;
        h = surface->h;

        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
        if(!tex) {
            SDL_Log("mx: Error creating texture from surface..");
            exit(EXIT_FAILURE);
        }
        SDL_FreeSurface(surface);
        return tex;
    }

    TTF_Font *mxUtil::loadFont(const std::string &filename, int size) {
        TTF_Font *fnt = TTF_OpenFont(getFilePath(filename).c_str(), size);
        if(!fnt) {
            SDL_Log("mx: Error Opening Font: %s", filename.c_str());
            exit(EXIT_FAILURE);
        }
        return fnt;
    }

    SDL_Surface *mxUtil::loadSurface(const std::string &name) {
        return  IMG_Load(getFilePath(name).c_str());
    }

    void mxUtil::initJoystick() {
        for(int i = 0; i < SDL_NumJoysticks(); ++i) {
            SDL_Joystick *stick_ = SDL_JoystickOpen(i);
            if(!stick_) {
                SDL_Log("mx: Joystick disabled..");
            } else {
                SDL_Log("mx: Joystick: %s enabled...", SDL_JoystickName(stick_));
            }
            stick.push_back(stick_);
        }
        SDL_JoystickEventState(SDL_ENABLE);
    }

    void mxUtil::closeJoystick() {
        for(int i = 0; i < SDL_NumJoysticks(); ++i) {
            SDL_JoystickClose(stick[i]);
        }
    }
    std::string LoadTextFile(const char* filename) {
        SDL_RWops* rw = SDL_RWFromFile(filename, "rb");
        if (!rw) {
            SDL_Log("Failed to open file '%s': %s", filename, SDL_GetError());
            return "";
        }
        Sint64 fileSize = SDL_RWsize(rw);
        if (fileSize < 0) {
            SDL_Log("Failed to get file size: %s", SDL_GetError());
            SDL_RWclose(rw);
            return "";
        }
        char* buffer = new char[fileSize + 1];
        if (!buffer) {
            SDL_Log("Memory allocation error");
            SDL_RWclose(rw);
            return "";
        }
        size_t bytesRead = SDL_RWread(rw, buffer, 1, fileSize);
        if (bytesRead != static_cast<size_t>(fileSize)) {
            SDL_Log("Failed to read the entire file: %s", SDL_GetError());
            delete[] buffer;
            SDL_RWclose(rw);
            return "";
        }
        buffer[fileSize] = '\0';
        std::string fileContents(buffer);
        delete[] buffer;
        SDL_RWclose(rw);
        return fileContents;
    }

}