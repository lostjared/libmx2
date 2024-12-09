#ifndef __INTRO_H__
#define __INTRO_H__

#include "mx.hpp"
#include "argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#endif

#include "gl.hpp"
#include "loadpng.hpp"
#include<memory>

class Intro : public gl::GLObject {
    GLuint VAO, VBO;
    GLuint texture;
    gl::ShaderProgram shaderProgram;
    mx::Controller stick;
public:
    Intro() = default;
    virtual ~Intro() override {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteTextures(1, &texture);
    }
    virtual void load(gl::GLWindow  *win) override;
    virtual void draw(gl::GLWindow *win) override;
    virtual void event(gl::GLWindow *win, SDL_Event &e) override;
};

#endif