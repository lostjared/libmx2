#include "vk.hpp"
#include"SDL.h"

#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
#include "argz.hpp"
#endif

class MainWindow : public mx::VKWindow {
public:
    MainWindow(const std::string& path, int wx, int wy, bool full) : mx::VKWindow("-[ VK Texture Mapped ]-", wx, wy, full) {
        setPath(path);
    }
    virtual ~MainWindow() {
    }
    virtual void event(SDL_Event& e) override {
        if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            quit();
    }
private:

};
int main(int argc, char **argv) {
    #if defined(__APPLE__) || defined(_WIN32) || defined(_WIN64) || defined(__linux__)
#ifndef __ANDROID__
        Arguments args = proc_args(argc, argv);
        try {
            MainWindow window(args.path, args.width, args.height, args.fullscreen);
            window.initVulkan();
            window.loop();   
            window.cleanup();
        } catch (mx::Exception &e) {
            SDL_Log("mx: Exception: %s\n", e.text().c_str());
        }
#endif
    #elif defined(__ANDROID__)
        try {
            MainWindow window("", 960, 720, false);
            window.initVulkan();
            window.loop();   
            window.cleanup();
        } catch (mx::Exception &e) {
            SDL_Log("mx: Exception: %s\n", e.text().c_str());
        }
    #endif
        return EXIT_SUCCESS;
}