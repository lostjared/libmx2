#ifndef __INTRO__H___
#define __INTRO__H___

#include"mx.hpp"
#include"gl.hpp"


class IntroScreen : public gl::GLObject {
    float fade = 0.0f;
public:
    IntroScreen() = default;
    ~IntroScreen() override {}
    virtual void load(gl::GLWindow *win) override;
    void draw(gl::GLWindow *win) override;
    void event(gl::GLWindow *win, SDL_Event &e) override {}
    void resize(gl::GLWindow *win, int w, int h) override;
    
private:
    Uint32 lastUpdateTime;
    gl::GLSprite logo;
    gl::ShaderProgram program;
};

#endif