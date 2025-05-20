#ifndef __START_H__
#define __START_H__

#include <iostream>
#include "mx.hpp"
#include "gl.hpp"

class Start : public gl::GLObject {
public:
    Start() {}
    virtual void load(gl::GLWindow *win) override {
        load_shader();
        start.initSize(win->w, win->h);
        start.loadTexture(&program, win->util.getFilePath("data/start.png"), 0.0f, 0.0f, win->w, win->h);
        win->console.print("Press any key to start\n");
    }
    void load_shader();
    virtual void event(gl::GLWindow *win, SDL_Event &e) override;
    virtual void draw(gl::GLWindow *win) override;
private:
    gl::ShaderProgram program;
    gl::GLSprite start;
    float fade = 0.0f;
    bool fade_in = true;
    Uint32 currentTime = 0, lastUpdateTime = 0;
    void setGame(gl::GLWindow *win);
};

#endif