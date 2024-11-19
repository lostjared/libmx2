#ifndef __MX__H___
#define __MX__H___

#include "SDL.h"
#include "SDL_ttf.h"
#include<iostream>
#include<string>
#include"util.hpp"
#include"object.hpp"
#include"font.hpp"
#include"tee_stream.hpp"
#include"texture.hpp"
#include"exception.hpp"
#include"joystick.hpp"
#include"wrapper.hpp"
#include<memory>

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
        void proc();
        void destroy() { setActive(false); }
        void setPath(const std::string &path) { util.path = path; }
        mxUtil util;
        Text text;
        std::unique_ptr<obj::Object> object;
        SDL_Renderer *renderer = nullptr;
        int width = 0, height = 0;
        void setObject(obj::Object *o);
    protected:
        SDL_Window *window = nullptr;
        void create_window(const std::string &name, int w, int h, bool full); 
        bool active = false; 
        SDL_Event e;
    };
}

#endif
