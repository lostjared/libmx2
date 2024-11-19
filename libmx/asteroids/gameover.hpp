#ifndef __GAME_OVER_H__
#define __GAME_OVER_H__

#include"mx.hpp"
#include"intro.hpp"

class GameOver : public obj::Object {
public:
    GameOver() {}
    virtual ~GameOver() {}

    virtual void load(mx::mxWindow* win) override {
        the_font.loadFont(win->util.getFilePath("data/font.ttf"), 14);
    }

    virtual void draw(mx::mxWindow* win) override {
        win->text.setColor({255,255,255,255});
        win->text.printText_Solid(the_font, 100, 100, "Game Over [ Press Enter to Play again ]");
    }

    virtual void event(mx::mxWindow* win, SDL_Event& e) override {
        if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN) {
            win->setObject(new Intro());
            win->object->load(win);
            return;
        }
    }

private:
    mx::Font the_font;
};


#endif