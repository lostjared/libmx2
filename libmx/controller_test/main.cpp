#include"mx.hpp"
#include<algorithm>
#include<unordered_map>
#include<chrono>
#include<sstream>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

class Intro : public obj::Object {
public:
    Intro() = default;
    ~Intro() override {}

    virtual void load(mx::mxWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 18);
        pos_x = 25;
        pos_y = win->height/2-(24/2);
        win_width = win->width;
        win_height = win->height;
        updateTitle(win);
    }

    virtual void draw(mx::mxWindow *win) override {
        SDL_Rect rect = { pos_x, pos_y, square_size, square_size };
        SDL_SetRenderDrawColor(win->renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(win->renderer, &rect);
        std::ostringstream stream;
        if(controller.active()) {
            stream <<"Controller: " << controller.name() << " connected at port: " << controller.controllerIndex();
        } else {
            stream << "Connect a controller";
        }
        win->text.printText_Blended(font, 10, 10, stream.str());
        auto now = std::chrono::steady_clock::now();
        for (auto &[button, timestamp] : buttonState) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp).count();
            if (elapsed >= repeat_delay) {
                handleButtonRepeat(button);
                buttonState[button] = now; 
             }
        }
    }

    virtual void event(mx::mxWindow *win, SDL_Event &e) override {
        auto now = std::chrono::steady_clock::now();

        if(controller.connectEvent(e)) {
            updateTitle(win);
        }

        if (e.type == SDL_CONTROLLERBUTTONDOWN) {
            buttonState[e.cbutton.button] = now; 
        }

        if (e.type == SDL_CONTROLLERBUTTONUP) {
            buttonState.erase(e.cbutton.button); 
        }
    }

private:

    void updateTitle(mx::mxWindow *win) {
        if(!controller.active())
            win->setWindowTitle("-[ Connect Controller ]-");
        else
            win->setWindowTitle("-[ Controller Connected ]-");
    }

    void handleButtonRepeat(Uint8 button) {
        if (button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
            pos_y -= movement_speed;
        } else if (button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
            pos_y += movement_speed;
        } else if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) {
            pos_x -= movement_speed;
        } else if (button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
            pos_x += movement_speed;
        }

        pos_x = std::max(0, std::min(win_width - square_size, pos_x));
        pos_y = std::max(0, std::min(win_height - square_size, pos_y));
    }

    int pos_x = 0, pos_y = 0;
    const int square_size = 50;
    const int movement_speed = 10;
    const int repeat_delay = 15; 
    int win_width = 640, win_height = 480;

    mx::Controller controller;
    mx::Font font;
    std::unordered_map<Uint8, std::chrono::steady_clock::time_point> buttonState;
};


class MainWindow : public mx::mxWindow {
public:
    MainWindow(const std::string &path) : mx::mxWindow("Controller Test", 960,  720, false) {
        setPath(path);
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
