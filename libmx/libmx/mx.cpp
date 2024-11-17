#include"mx.hpp"
#ifdef FOR_WASM
#include<emscripten/emscripten.h>
#endif
namespace mx {

    mxWindow::mxWindow(const std::string &name, int w, int h, bool full) {
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
            std::cerr << "mx: Error initalizing SDL: " << SDL_GetError() << "\n";
            std::cerr.flush();
            exit(EXIT_FAILURE);
        }
        if(TTF_Init() < 0) {
            std::cerr << "mx: Error could not initalize SDL_ttf...\n";
            std::cerr.flush();
            exit(EXIT_FAILURE);
        }
        create_window(name, w, h, full);
        text.setRenderer(renderer);
        text.setColor({255,255,255,255});
    }

    void mxWindow::setObject(obj::Object *o) {
        object.reset(o);
    }
    
    mxWindow::~mxWindow() {
        object.reset();
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        TTF_Quit();
        SDL_Quit();
    }
    SDL_Texture *mxWindow::createTexture(int w, int h) {
        SDL_Texture *tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
        if(!tex) {
            std::cerr << "mx: Error creating texture: " << SDL_GetError() << "\n";
            std::cerr.flush();
            exit(EXIT_FAILURE);
        }
        return tex;
    }

    void mxWindow::create_window(const std::string &name, int w, int h, bool full) {
        window = SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, full == true ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_SHOWN);
        if (!window) {
            std::cerr << "mx: Error initalzing Window: " << SDL_GetError() << "\n";
            std::cerr.flush();
            SDL_Quit();
            exit(EXIT_FAILURE);
        }
        width = w;
        height = h;
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            std::cerr << "mx: Error Creating Renderer: " << SDL_GetError() << "\n";
            std::cerr.flush();
            SDL_DestroyWindow(window);
            SDL_Quit();
            exit(EXIT_FAILURE);
        }

    }

    void mxWindow::loop() {

        if(!object) {
            std::cerr << "mx: Requires you set an object..\n";
            std::cerr.flush();
            exit(EXIT_FAILURE);
        }

        SDL_Event e;
        active = true;
        while(active) {
            while(SDL_PollEvent(&e)) {
                if(e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
                    active = false;

                event(e);
                object->event(this, e);
            }
            draw(renderer);
        }
    }
}