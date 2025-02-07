#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_image.h"
#include"gl.hpp"


class Object : public gl::GLObject {
public:

     virtual void load(gl::GLWindow *win) {

     }

     virtual void draw(gl::GLWindow *win) {

     }

     virtual void event(gl::GLWindow *win, SDL_Event  &e) {

     }

};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(int wx, int wy) : gl::GLWindow("OpenGL Demo", wx , wy) {
         setObject(new Object());
    }

    virtual void draw() {
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        swap();
    }

    virtual void event(SDL_Event  &e) {
       
    }
};

int main(int argc, char* argv[]) {
    MainWindow window(800, 600);
    window.loop();
    return 0;
}

