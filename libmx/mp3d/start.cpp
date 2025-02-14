#include"start.hpp"

void Start::draw(gl::GLWindow *win) {
    glDisable(GL_DEPTH_TEST);
    program.useProgram();
    program.setUniform("alpha", fade);
    start.draw();
    currentTime = SDL_GetTicks();
    if((currentTime - lastUpdateTime) > 25) {
        lastUpdateTime = currentTime;
        if(fade_in == true && fade < 1.0f) fade += 0.05;
        if(fade_in == false && fade > 0.1f) fade -= 0.05;
    }
    if(fade_in == true && fade >= 1.0f) {
        fade = 1.0f;
    } else if(fade_in == false  && fade <= 0.1f) {
        fade = 0.0f;
        setGame(win);
        return;
    }
}

