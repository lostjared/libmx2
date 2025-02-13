#include"start.hpp"

void Start::draw(gl::GLWindow *win) {
    glDisable(GL_DEPTH_TEST);
    program.useProgram();
    start.draw();
}

void Start::load_shader() {
    if(!program.loadProgramFromText(gl::vSource, gl::fSource)) {
        throw mx::Exception("Failed to load shader program Intro::load_shader()");
    }
}