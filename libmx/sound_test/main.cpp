#include"mx.hpp"
#ifdef __EMSCRIPTEN__
#include<emscripten/emscripten.h>
#endif


class Intro : public obj::Object {
public:
    Intro() = default;
    ~Intro() override { 
    
    }

    virtual void load(mx::mxWindow *win) override {

    }

    virtual void draw(mx::mxWindow *win) override {
        for(int i = 0; i < win->width; i += 32) {
            for(int z = 0; z < win->height; z += 32) {
                SDL_Rect rc = {i, z, 32, 32};
                SDL_SetRenderDrawColor(win->renderer, rand()%255, rand()%255, rand()%255, 255);
                SDL_RenderFillRect(win->renderer, &rc);
            }
        }
    }

    virtual void event(mx::mxWindow *win, SDL_Event &e) override  {

    }
};


class MainWindow : public mx::mxWindow {
public:
    MainWindow(std::string path) : mx::mxWindow("Hello World", 640, 480, false) {
        setPath(path);
#ifdef WITH_MIXER
        mixer.init();
        loadSound();
#endif
        setObject(new Intro());
        object->load(this);
    }
    int sound1 = 0;
    void loadSound() {
        #ifdef WITH_MIXER
            sound1 = mixer.loadMusic(util.getFilePath("data/output.ogg"));
            mixer.playMusic(sound1, 0);
        #endif
    }
     ~MainWindow() override {

     }
    virtual void event(SDL_Event &e) override {
    }
    virtual void draw(SDL_Renderer *renderer) override {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // draw
        object->draw(this);
        SDL_RenderPresent(renderer);
    }
};


MainWindow *main_w = nullptr;

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    try {
        MainWindow main_window("/");
        main_w =&main_window;
        emscripten_set_main_loop(eventProc, 0, 1);
    } catch(const mx::Exception &e) {
        std::cout << e.text() << "\n";
    }
#else
    try {
        std::string path = ".";
        if(argc == 2) {
            path = argv[1];    
        } else {
            mx::system_err << "Requires one argument, being path to resouces...\n";
            mx::system_out << "Trying default path . \n";
            path = ".";
        }
        MainWindow main_window(path);
        main_window.loop();
    } 
    catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif

    return 0;
}
