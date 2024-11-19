#ifndef __INTRO_H__
#define __INTRO_H__

#include "mx.hpp"
#include "game.hpp"

class Intro : public obj::Object {
public:
    Intro() {}
    virtual ~Intro()  {}
    virtual void load(mx::mxWindow *win) override {
        tex.loadTexture(win, win->util.getFilePath("data/logo.png"));
    }

    virtual void draw(mx::mxWindow *win) override {
        static Uint32 previous_time = SDL_GetTicks();
        Uint32 current_time = SDL_GetTicks();
        SDL_SetTextureAlphaMod(tex.wrapper().unwrap(), alpha);
        SDL_RenderCopy(win->renderer, tex.wrapper().unwrap(), nullptr, nullptr);
        if(done == true) {
            win->setObject(new Game());
            win->object->load(win);
            return;
        }        
        if (current_time - previous_time >= 15) {
            previous_time = current_time;
            if (fading_out) {
                alpha -= 3;  
                if (alpha <= 0) {
                    alpha = 0;
                    fading_out = false;
                    done = true;
                }
            } else {
                alpha += 3;
                if (alpha >= 255) {
                    alpha = 255;
                    fading_out = true;
                    previous_time = SDL_GetTicks();
                }
            }
        }
    }
    virtual void event(mx::mxWindow *win, SDL_Event &e) override {}
private:
    mx::Texture tex;
    int alpha = 255;
    bool fading_out = true;
    bool done = false;
};

#endif