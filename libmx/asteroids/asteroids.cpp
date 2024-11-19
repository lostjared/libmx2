#include"mx.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "intro.hpp"

class MainWindow : public mx::mxWindow {
public:
    
    MainWindow(std::string path) : mx::mxWindow("Asteroids", 960, 720, false) {
        tex.createTexture(this, 640, 480);
      	setPath(path);
        setObject(new Intro());
		object->load(this);
    }
    
    ~MainWindow() override {

    }
    
    virtual void event(SDL_Event &e) override {}

    virtual void draw(SDL_Renderer *renderer) override {
        SDL_SetRenderTarget(renderer, tex.wrapper().unwrap());
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // draw
        object->draw(this);
        SDL_SetRenderTarget(renderer,nullptr);
        SDL_RenderCopy(renderer, tex.wrapper().unwrap(), nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }
private:
    mx::Texture tex;
};

MainWindow *main_w = nullptr;

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    MainWindow main_window("/");
    main_w =&main_window;
    emscripten_set_main_loop(eventProc, 0, 1);
#else
	if(argc == 2) {
    	MainWindow main_window(argv[1]);
    	main_window.loop();
	} else {
        MainWindow main_window(".");
        main_window.loop();
	}
    return 0;
#endif
}