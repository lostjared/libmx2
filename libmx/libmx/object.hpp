#ifndef __GAME_OBJECT__H_
#define __GAME_OBJECT__H_

#include "SDL.h"
#include<memory>

namespace mx {
    class mxWindow;
}
namespace obj {
    class Object {
    public:
        virtual ~Object() = default;
        virtual void draw(mx::mxWindow *win) = 0;
        virtual void event(mx::mxWindow *win, SDL_Event &e) = 0;
        virtual void load(mx::mxWindow *renderer) = 0;
    };
}

#endif