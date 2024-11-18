#include"mx.hpp"


class Intro : public obj::Object {
public:
    Intro() = default;
    virtual ~Intro() = default;

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
    MainWindow() : mx::mxWindow("Hello World", 640, 480, false) {
        setObject(new Intro());
        object->load(this);
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

int main(int argc, char **argv) {
    MainWindow main_window;
    main_window.loop();
    return 0;
}
