#include"mx.hpp"
#ifdef FOR_WASM
#include<emscripten/emscripten.h>
#endif
namespace mx {

    mxWindow::mxWindow(const std::string &name, int w, int h, bool full) {
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
            mx::system_err << "mx: Error initalizing SDL: " << SDL_GetError() << "\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
        }
        if(TTF_Init() < 0) {
            mx::system_err << "mx: Error could not initalize SDL_ttf...\n";
            mx::system_err.flush();
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
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }
    SDL_Texture *mxWindow::createTexture(int w, int h) {
        SDL_Texture *tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
        if(!tex) {
            mx::system_err << "mx: Error creating texture: " << SDL_GetError() << "\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
        }
        return tex;
    }

    void mxWindow::create_window(const std::string &name, int w, int h, bool full) {
        window = SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, full == true ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_SHOWN);
        if (!window) {
            mx::system_err << "mx: Error initalzing Window: " << SDL_GetError() << "\n";
            mx::system_err.flush();
            SDL_Quit();
            exit(EXIT_FAILURE);
        }
        width = w;
        height = h;
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            mx::system_err << "mx: Error Creating Renderer: " << SDL_GetError() << "\n";
            mx::system_err.flush();
            SDL_DestroyWindow(window);
            SDL_Quit();
            exit(EXIT_FAILURE);
        }

    }

    void mxWindow::loop() {
        if(!object) {
            throw Exception("Requires you set an Object");
        }
        active = true;
        while(active) {
            proc();
        }
    }

    void mxWindow::proc() {

        if(!object) {
            throw Exception("Requires an active Object");
        }

        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
                active = false;

            event(e);
            object->event(this, e);
        }
        draw(renderer);
    }
}