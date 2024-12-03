#include"mx.hpp"
#include<algorithm>

#include <unordered_map>
#include <chrono>

class Intro : public obj::Object {
public:
    Intro() = default;
    ~Intro() override {}

    virtual void load(mx::mxWindow *win) override {
        if (controller.open(0)) {
            mx::system_out << "Controller opened: " << controller.name() << "\n";
        } else {
            mx::system_out << "Failed to open controller.\n";
        }
    }

    virtual void draw(mx::mxWindow *win) override {
        SDL_Rect rect = { pos_x, pos_y, square_size, square_size };
        SDL_SetRenderDrawColor(win->renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(win->renderer, &rect);

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

        if (e.type == SDL_CONTROLLERBUTTONDOWN) {
            buttonState[e.cbutton.button] = now; 
        }

        if (e.type == SDL_CONTROLLERBUTTONUP) {
            buttonState.erase(e.cbutton.button); 
        }
    }

private:
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
    std::unordered_map<Uint8, std::chrono::steady_clock::time_point> buttonState;
};


class MainWindow : public mx::mxWindow {
public:
    MainWindow() : mx::mxWindow("Controller Test", 640, 480, false) {
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
    try {
        MainWindow main_window;
        main_window.loop();
    } 
    catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
    return 0;
}
