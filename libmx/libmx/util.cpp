#include"util.hpp"
#include"loadpng.hpp"
#include<sstream>

namespace mx {

    std::string mxUtil::getFilePath(const std::string &filename) {
        std::ostringstream stream;
        stream << path << "/" << filename;
        return stream.str();
    }

    void mxUtil::printText(SDL_Renderer *renderer,TTF_Font *font,int x, int y, const std::string &text, SDL_Color col) {
        SDL_Surface *surf = TTF_RenderText_Blended(font,text.c_str(), col);
        if(!surf) {
            mx::system_err << "mx: Error rendering text...\n";
            return;
        }
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        if(!tex) {
            mx::system_err << "mx: Error creating texture..\n";
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
        SDL_Surface *surface = png::LoadPNG(getFilePath(filename).c_str());
        if(!surface) {
            mx::system_err << "mx: Error could not open file: " << getFilePath(filename) << "\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
        }
        if(color)
            SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, key.r, key.g, key.b));

        w = surface->w;
        h = surface->h;

        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
        if(!tex) {
            mx::system_err << "mx: Error creating texture from surface..\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
        }
        SDL_FreeSurface(surface);
        return tex;
    }

    TTF_Font *mxUtil::loadFont(const std::string &filename, int size) {
        TTF_Font *fnt = TTF_OpenFont(getFilePath(filename).c_str(), size);
        if(!fnt) {
            mx::system_err << "mx: Error Opening Font: " << filename << "\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
        }
        return fnt;
    }

    SDL_Surface *mxUtil::loadSurface(const std::string &name) {
        return png::LoadPNG(getFilePath(name).c_str());
    }

    void mxUtil::initJoystick() {
        for(int i = 0; i < SDL_NumJoysticks(); ++i) {
            SDL_Joystick *stick_ = SDL_JoystickOpen(i);
            if(!stick_) {
                mx::system_out << "mx: Joystick disabled..\n";
            } else {
                mx::system_out << "mx: Joystick: " << SDL_JoystickName(stick_) << " enabled...\n";
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

}