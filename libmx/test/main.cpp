#include"mx.h"

class MainWindow : public mx::mxWindow {
public:
    MainWindow() : mx::mxWindow("Hello World", 640, 480, false) {

    }
    
    virtual void event(SDL_Event &e) override {
        
    }

    virtual void draw(SDL_Renderer *renderer) override {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // draw
        SDL_RenderPresent(renderer);
    }
};

int main(int argc, char **argv) {
    MainWindow main_window;
    main_window.loop();
    return 0;
}