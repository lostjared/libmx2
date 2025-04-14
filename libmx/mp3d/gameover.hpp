#ifndef __GAME_OVER__H_
#define __GAME_OVER__H_

#include <iostream>
#include "mx.hpp"
#include "gl.hpp"

class GameOver : public gl::GLObject {
public:
    GameOver(int score, int level) : score_{score}, level_{level} {
    }
    virtual void load(gl::GLWindow *win) override {
        load_shader();
        game_over.initSize(win->w, win->h);
        game_over.loadTexture(&program, win->util.getFilePath("data/mp_wall.png"), 0.0f, 0.0f, win->w, win->h);
        font.loadFont(win->util.getFilePath("data/font.ttf"), 75);
        win->console.printf("Game Over Score: %d level: %d\n", score_, level_);
    }
    void load_shader();
    virtual void event(gl::GLWindow *win, SDL_Event &e) override;
    virtual void draw(gl::GLWindow *win) override;
private:
    gl::ShaderProgram program;
    gl::GLSprite game_over;
    Uint32 lastUpdateTime = 0;
    float fade = 0.0f;
    int score_ = 0, level_ = 0;
    mx::Font font;
};

#endif