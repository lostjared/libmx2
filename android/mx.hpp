#ifndef __MX__H___
#define __MX__H___

#ifdef __EMSCRIPTEN__
#include "config.hpp"
#else
#include "config.h"
#endif

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include "exception.hpp"
#include "font.hpp"
#include "input.hpp"
#include "joystick.hpp"
#include "loadpng.hpp"
#include "object.hpp"
#include "tee_stream.hpp"
#include "texture.hpp"
#include "util.hpp"
#include "wrapper.hpp"
#include <iostream>
#include <memory>
#include <string>
#ifdef WITH_MIXER
#include "sound.hpp"
#endif

namespace mx {

    class mxWindow {
      public:
        std::unique_ptr<obj::Object> object;
        mxWindow() = delete;
        mxWindow(const std::string &name, int w, int h, bool full = false);
        virtual ~mxWindow();
        virtual void event(SDL_Event &e) = 0;
        virtual void draw(SDL_Renderer *ren) = 0;
        SDL_Texture *createTexture(int w, int h);
        void setActive(bool a) { active = a; }
        void loop();
        void proc();
        void destroy() { setActive(false); }
        void setPath(const std::string &path) { util.path = path; }
        void setIcon(SDL_Surface *icon);
        void setIcon(const std::string &icon);
        void setFullScreen(bool full);
        void quit();
        mxUtil util;
        Text text;
        SDL_Renderer *renderer = nullptr;
        int width = 0, height = 0;
        void setObject(obj::Object *o);
        SDL_Window *window = nullptr;
        void setWindowTitle(const std::string &title);
#ifdef WITH_MIXER
        Mixer mixer;
#endif
      protected:
        void create_window(const std::string &name, int w, int h, bool full);
        bool active = false;
        SDL_Event e;
    };
} // namespace mx

#endif
