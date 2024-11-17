#include"mx.hpp"


class Intro : public obj::Object {
public:
    Intro() = default;
    virtual ~Intro() = default;

    virtual void load(SDL_Renderer *renderer) override {

    }

    virtual void draw(SDL_Renderer *renderer) override {

    }

    virtual void event(SDL_Renderer *renderer, SDL_Event &e) override  {

    }
};


class MainWindow : public mx::mxWindow {
public:
    MainWindow() : mx::mxWindow("Hello World", 640, 480, false) {
        setObject(new Intro());
    }
    
    virtual ~MainWindow() = default;

    virtual void event(SDL_Event &e) override {
        
    }

    virtual void draw(SDL_Renderer *renderer) override {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // draw
        object->draw(renderer);
        SDL_RenderPresent(renderer);
    }
};

int main(int argc, char **argv) {
    MainWindow main_window;
    main_window.loop();
    return 0;
}
