#ifndef _INTRO_H__
#define _INTRO_H__

#include <iostream>
#include "mx.hpp"
#include "gl.hpp"

class Intro : public gl::GLObject {
public:
    Intro() {}
    virtual void load(gl::GLWindow *win) override {
        load_shader();
        intro.initSize(win->w, win->h);
        intro.loadTexture(&program, win->util.getFilePath("data/intro.png"), 0.0f, 0.0f, win->w, win->h);
    }
    void load_shader();
    virtual void event(gl::GLWindow *win, SDL_Event &e) override;
    virtual void draw(gl::GLWindow *win) override;
private:
    gl::ShaderProgram program;
    gl::GLSprite intro;
    Uint32 lastUpdateTime = 0;
    float fade = 1.0f;
};


#endif