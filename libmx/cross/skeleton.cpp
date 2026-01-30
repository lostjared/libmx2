#include <SDL2/SDL.h>
#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
#include "argz.hpp"
#endif

#include "mxcross.hpp"


class App : public MX_App {
public:
    void load(MX_WindowBase *win) override {
        
    }

    void draw(MX_WindowBase *win) override {
        
    }

    void resize(MX_WindowBase *win, int w, int h) override {
        
    }

    void event(MX_WindowBase *win, SDL_Event &e) override {
        if (e.type == SDL_QUIT) {
            win->quit();
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
            win->quit();
        }
    }
};

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
    #if defined(WITH_VULKAN) || defined(VULKAN) || defined(USE_VULKAN) || defined(WITH_VK)
    auto window = create_vk_window();
    #else
    auto window = create_gl_window();
    #endif
        App app;
        window->setPath(args.path);
        window->setApp(&app);
        window->run("App", args.width, args.height, args.fullscreen);
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
    return EXIT_SUCCESS;
}
