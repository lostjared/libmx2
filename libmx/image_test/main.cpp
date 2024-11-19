#include"mx.hpp"

class Intro : public obj::Object {
public:
    Intro() = default;
    ~Intro() override { 
    
    }
    virtual void load(mx::mxWindow *win) override {
        texture.loadTexture(win, win->util.getFilePath("img/logo.png"));
	}
    virtual void draw(mx::mxWindow *win) override {
        SDL_RenderCopy(win->renderer, texture.wrapper().unwrap(), nullptr, nullptr);
    }
    virtual void event(mx::mxWindow *win, SDL_Event &e) override  {}
private:
	mx::Texture texture;
};


class MainWindow : public mx::mxWindow {
public:
    MainWindow(std::string path) : mx::mxWindow("Hello World", 640, 480, false) {
      	setPath(path);
        setObject(new Intro());
		object->load(this);
    }
    ~MainWindow() override {

    }
    virtual void event(SDL_Event &e) override {}
    virtual void draw(SDL_Renderer *renderer) override {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // draw
        object->draw(this);
        SDL_RenderPresent(renderer);
    }
};

int main(int argc, char **argv) {
	try {
        MainWindow main_window(path, tw, th);
#ifdef __EMSCRIPTEN__
        main_win =  &main_window;
        emscripten_set_main_loop(eventProc, 0, 1);
#else
        main_window.loop();
#endif
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: " << e.text() << "\n";
    }
    return 0;
}