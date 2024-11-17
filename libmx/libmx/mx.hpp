#ifndef __MX__H___
#define __MX__H___

#include "SDL.h"
#include "SDL_ttf.h"
#include<iostream>
#include<string>
#include<functional>

namespace mx {

    class mxWindow {
    public:
        mxWindow() = delete;
        mxWindow(const std::string &name, int w, int h, bool full);
        virtual ~mxWindow();
        virtual void event(SDL_Event &e) = 0;
        virtual void draw(SDL_Renderer *ren) = 0;
        SDL_Texture *createTexture(int w, int h);
        void setActive(bool a) { active = a; }
        void loop();
        void destroy() { setActive(false); }
    protected:
        SDL_Window *window;
        SDL_Renderer *renderer;
        void create_window(const std::string &name, int w, int h, bool full); 
        bool active = false;      
    };

}

#endif
