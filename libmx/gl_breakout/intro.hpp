#ifndef __INTRO_H__
#define __INTRO_H__

#include "argz.hpp"
#include "mx.hpp"

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include "gl.hpp"
#include "loadpng.hpp"
#include "model.hpp"
#include <memory>

class Intro : public gl::GLObject {
    gl::ShaderProgram shaderProgram;
    mx::Controller stick;
    mx::Model cube;
    GLuint texture;
    gl::GLSprite sprite;
    gl::ShaderProgram program;
    mx::Font font;

  public:
    Intro() = default;
    virtual ~Intro() override {}
    virtual void load(gl::GLWindow *win) override;
    virtual void draw(gl::GLWindow *win) override;
    virtual void event(gl::GLWindow *win, SDL_Event &e) override;
};

#endif
