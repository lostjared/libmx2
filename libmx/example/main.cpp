#include"mx.hpp"

class Intro : public obj::Object {
public:
    Intro() = default;
    virtual ~Intro() = default;
    virtual void load(mx::mxWindow *win) override {
		font.loadFont("font.ttf", 14);
	}
    virtual void draw(mx::mxWindow *win) override {
        win->text.printText_Solid(font, 25, 25, "Hello, World!");
    }
    virtual void event(mx::mxWindow *win, SDL_Event &e) override  {}
private:
	mx::Font font;
};


class MainWindow : public mx::mxWindow {
public:
    MainWindow() : mx::mxWindow("Hello World", 640, 480, false) {
        setObject(new Intro());
		object->load(this);
    }
    virtual ~MainWindow() = default;
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
    MainWindow main_window;
    main_window.loop();
    return 0;
}