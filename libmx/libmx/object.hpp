#ifndef __GAME_OBJECT__H_
#define __GAME_OBJECT__H_

#include "SDL.h"
#include<memory>

namespace obj {
    class Object {
    public:
        virtual ~Object() = default;
        virtual void draw(SDL_Renderer *renderer) = 0;
        virtual void event(SDL_Renderer *renderer, SDL_Event &e) = 0;
        virtual void load(SDL_Renderer *renderer) = 0;
    };
}

#endif