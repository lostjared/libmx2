#include "vk.hpp"
#include "SDL.h"

#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
#include "argz.hpp"
#endif

class SkeletonWindow : public mx::VKWindow {
public:
    SkeletonWindow(const std::string& path, int wx, int wy, bool full) 
        : mx::VKWindow("-[ Vulkan Skeleton ]-", wx, wy, full) {
        setPath(path);
    }
    
    virtual ~SkeletonWindow() {}
    
    void initVulkan() override {
        mx::VKWindow::initVulkan();
    }
    
    void proc() override {
        printText("Hello World", 50, 50, {255, 255, 255, 255});
    }
    
    void event(SDL_Event& e) override {
        if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
            quit();
        }
    }

private:
};

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
        SkeletonWindow window(args.path, args.width, args.height, args.fullscreen);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
    return EXIT_SUCCESS;
}
