#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_image.h"
#include"gl.hpp"


class Object : public gl::GLObject {
public:

     virtual void load(gl::GLWindow *win) {
        font.loadFont("font.ttf", 36);
     }

     virtual void draw(gl::GLWindow *win) {
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 0.0f, 0.0f, "Hello World with OpenGL ES 3.0");
     }

     virtual void event(gl::GLWindow *win, SDL_Event  &e) {

     }

private:
     mx::Font font;

};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(int wx, int wy) : gl::GLWindow("OpenGL Demo", wx , wy) {
         setObject(new Object());
         object->load(this);
         
    }

    virtual void draw() {
        SDL_GL_GetDrawableSize(window, &this->w, &this->h);
        glViewport(0, 0, this->w, this->h);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        object->draw(this);
        swap();
    }

    virtual void event(SDL_Event  &e) {
        if (e.type == SDL_WINDOWEVENT) {
            if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                SDL_GL_GetDrawableSize(window, &this->w, &this->h);
                glViewport(0, 0, this->w, this->h);                
            }
        }
    }
};

int main(int argc, char* argv[]) {
    MainWindow window(800, 600);
    window.loop();
    return 0;
}

