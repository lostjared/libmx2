#include"mx.hpp"

class Intro : public obj::Object {
public:
    Intro() = default;
    virtual ~Intro() = default;
    virtual void load(mx::mxWindow *win) override {
        texture.loadTexture(win, win->util.getFilePath("img/logo.png"));
	}
    virtual void draw(mx::mxWindow *win) override {
        SDL_RenderCopy(win->renderer, texture.handle(), nullptr, nullptr);
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
	if(argc == 2) {
    	MainWindow main_window(argv[1]);
    	main_window.loop();
	} else {
		mx::system_err << "Error requies one argument to path of font.\n";
	}
    return 0;
}