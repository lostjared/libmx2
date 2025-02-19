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
#include"intro.hpp"


class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Breakout Game", tw, th) {
        setPath(path);
#ifdef WITH_MIXER
        mixer.init();
#endif
        //setObject(new BreakoutGame());
        setObject(new Intro());
        object->load(this);
    }

    ~MainWindow() override {}

    virtual void event(SDL_Event &e) override {
        object->event(this, e);
    }

    virtual void draw() override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, w, h);
        object->draw(this);
        swap();
        delay();
    }
};

MainWindow *main_w = nullptr;

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    MainWindow main_window("/", 960, 720);
    main_w = &main_window;
    emscripten_set_main_loop(eventProc, 0, 1);
#elif defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
    Arguments args = proc_args(argc, argv);
    try {
        MainWindow main_window(args.path, args.width, args.height);
        if (args.fullscreen)
            main_window.setFullScreen(true);
        main_window.loop();
    } catch (const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
