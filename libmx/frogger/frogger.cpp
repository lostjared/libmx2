#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

class Game : public obj::Object {
public:
    static constexpr int width = 640, height = 480;

    Game() = default;
    ~Game() override {}

    virtual void load(mx::mxWindow *win) override {
        frog = { width / 2 - 16, height - 64, 32, 32 };
        frogColor = { 0, 255, 0, 255 };
        int laneHeight = 64;
        for (int i = 1; i <= 4; ++i) {
            vehicles.push_back({ 0, i * laneHeight, 64, 32 });
            vehicleSpeeds.push_back(i * 2);
        }
        the_font.loadFont(win->util.getFilePath("data/font.ttf"), 14);
        lastUpdateTime = SDL_GetTicks();
    }

    virtual void draw(mx::mxWindow *win) override {
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastUpdateTime >= 15) {
            update();
            lastUpdateTime = currentTime;
        }

        SDL_Renderer *renderer = win->renderer;
        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
        for (int i = 1; i <= 4; ++i) {
            SDL_Rect lane = { 0, i * 64, width, 64 };
            SDL_RenderFillRect(renderer, &lane);
        }
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        for (const auto &vehicle : vehicles) {
            SDL_RenderFillRect(renderer, &vehicle);
        }
        SDL_SetRenderDrawColor(renderer, frogColor.r, frogColor.g, frogColor.b, frogColor.a);
        SDL_RenderFillRect(renderer, &frog);
        win->text.setColor({255,255,255,255});
        win->text.printText_Solid(the_font, 15, 15, "Other Side: " + std::to_string(side));
        win->text.printText_Solid(the_font, 15, 35, "Death: " + std::to_string(death));
    }

    virtual void event(mx::mxWindow *win, SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP:
                    frog.y -= 32;
                    break;
                case SDLK_DOWN:
                    if (frog.y + frog.h < height) frog.y += 32;
                    break;
                case SDLK_LEFT:
                    if (frog.x > 0) frog.x -= 32;
                    break;
                case SDLK_RIGHT:
                    if (frog.x + frog.w < width) frog.x += 32;
                    break;
            }
        }
    }

private:
    SDL_Rect frog;
    SDL_Color frogColor;
    std::vector<SDL_Rect> vehicles;
    std::vector<int> vehicleSpeeds;
    Uint32 lastUpdateTime;
    mx::Font the_font;
    int side = 0, death = 0;

    void update() {
        for (size_t i = 0; i < vehicles.size(); ++i) {
            vehicles[i].x += vehicleSpeeds[i];
            if (vehicles[i].x > width) {
                vehicles[i].x = -vehicles[i].w;
            }
        }
        for (const auto &vehicle : vehicles) {
            if (SDL_HasIntersection(&frog, &vehicle)) {
                resetFrog();
                death ++;
            }
        }
        if (frog.y < 0) {
            resetFrog();
            side ++;
        }
    }

    void resetFrog() {
        frog.x = width / 2 - frog.w / 2;
        frog.y = height - 64;
    }
};


class Intro : public obj::Object {
public:
    Intro() {}
    ~Intro() override {}

    virtual void load(mx::mxWindow *win) override {
        tex.loadTexture(win, win->util.getFilePath("data/logo.png"));
    }

    virtual void draw(mx::mxWindow *win) override {
        static Uint32 previous_time = SDL_GetTicks();
        Uint32 current_time = SDL_GetTicks();
        static int alpha = 255;
        static bool fading_out = true;
        static bool done = false;

        SDL_SetTextureAlphaMod(tex.wrapper().unwrap(), alpha);
        SDL_RenderCopy(win->renderer, tex.wrapper().unwrap(), nullptr, nullptr);

        if(done == true) {
            win->setObject(new Game());
            win->object->load(win);
            return;
        }
        
        if (current_time - previous_time >= 15) {
            previous_time = current_time;
            if (fading_out) {
                alpha -= 3;  
                if (alpha <= 0) {
                    alpha = 0;
                    fading_out = false;
                    done = true;
                }
            } else {
                alpha += 3;
                if (alpha >= 255) {
                    alpha = 255;
                    fading_out = true;
                    previous_time = SDL_GetTicks();
                }
            }
        }
    }

    virtual void event(mx::mxWindow *win, SDL_Event &e) override {

    }
private:
    mx::Texture tex;
};

class MainWindow : public mx::mxWindow {
public:
    MainWindow(std::string path, int tw, int th) : mx::mxWindow("Frogger", tw, th, false) {
        tex.createTexture(this, 640, 480);
      	setPath(path);
        setObject(new Intro());
		object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {}

    virtual void draw(SDL_Renderer *renderer) override {
        SDL_SetRenderTarget(renderer, tex.wrapper().unwrap());
        SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
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
    int tw = 640, th = 480;
#ifdef __EMSCRIPTEN__
    MainWindow main_window("/", tw, th);
    main_w =&main_window;
    emscripten_set_main_loop(eventProc, 0, 1);
#else
    Argz<std::string> parser(argc, argv);    
    parser.addOptionSingle('h', "Display help message")
          .addOptionSingleValue('p', "assets path")
          .addOptionDoubleValue('P', "path", "assets path")
          .addOptionSingleValue('r',"Resolution WidthxHeight")
          .addOptionDoubleValue('R',"resolution", "Resolution WidthxHeight");
    Argument<std::string> arg;
    std::string path;
    int value = 0;
    try {
        while((value = parser.proc(arg)) != -1) {
            switch(value) {
                case 'h':
                case 'v':
                    parser.help(std::cout);
                    exit(EXIT_SUCCESS);
                    break;
                case 'p':
                case 'P':
                    path = arg.arg_value;
                break;
                case 'r':
                case 'R': {
                    auto pos = arg.arg_value.find("x");
                    if(pos == std::string::npos)  {
                        mx::system_err << "Error invalid resolution use WidthxHeight\n";
                        mx::system_err.flush();
                        exit(EXIT_FAILURE);
                    }
                    std::string left, right;
                    left = arg.arg_value.substr(0, pos);
                    right = arg.arg_value.substr(pos+1);
                    tw = atoi(left.c_str());
                    th = atoi(right.c_str());
                }
                break;
            }
        }
    } catch (const ArgException<std::string>& e) {
        mx::system_err << e.text() << "\n";
    }
    if(path.empty()) {
        mx::system_out << "mx: No path provided trying default current directory.\n";
        path = ".";
    }
    try {
        MainWindow main_window(path, tw, th);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    } 
#endif
    return 0;
}