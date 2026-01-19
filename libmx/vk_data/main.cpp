#include "vk.hpp"
#include"SDL.h"

#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
#include "argz.hpp"
#endif

struct Vertex {
    float pos[3];
    float texCoord[2];
};

class MainWindow : public mx::VKWindow {
public:
    MainWindow(const std::string& path, int wx, int wy) : mx::VKWindow("-[ Image with Vulkan ]-", wx, wy) {
        setPath(path);
    }
    virtual ~MainWindow() {
    }
    virtual void event(SDL_Event& e) override {}

private:
 
};
int main(int argc, char **argv) {
    #if defined(__APPLE__) || defined(_WIN32) || defined(_WIN64) || defined(__linux__)
#ifndef __ANDROID__
        Arguments args = proc_args(argc, argv);
        try {
            MainWindow window(args.path, args.width, args.height);
            window.initVulkan();
            window.loop();   
            window.cleanup();
        } catch (mx::Exception &e) {
            SDL_Log("mx: Exception: %s\n", e.text().c_str());
        }
#endif
    #elif defined(__ANDROID__)
        try {
            MainWindow window("", 960, 720);
            window.initVulkan();
            window.loop();   
            window.cleanup();
        } catch (mx::Exception &e) {
            SDL_Log("mx: Exception: %s\n", e.text().c_str());
        }
    #endif
        return EXIT_SUCCESS;
}