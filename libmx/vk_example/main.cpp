#include "vk.hpp"
#include "SDL.h"

#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
#include "argz.hpp"
#endif

class ExampleWindow : public mx::VKWindow {
public:
    ExampleWindow(const std::string& path, int wx, int wy, bool full)
        : mx::VKWindow("-[ Vulkan Example / Sprite ]-", wx, wy, full) {
        setPath(path);
    }
    void initVulkan() override {
        mx::VKWindow::initVulkan();
        sprite = createSprite(util.path + "/data/logo.png",
                              util.path + "/data/sprite_vert.spv",
                              util.path + "/data/sprite_frag.spv");
    }
    void proc() override {
        if (!sprite) {
            return;
        }
        int sw = sprite->getWidth();
        int sh = sprite->getHeight();
        if (sw <= 0 || sh <= 0) {
            return;
        }
        sprite->drawSpriteRect(0, 0, getWidth(), getHeight());
        printText("Hello World", 15, 15, {255, 255, 255, 255});
    }
    void event(SDL_Event& e) override {
        if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
            quit();
        }
    }

private:
    mx::VKSprite* sprite = nullptr;
};

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
        ExampleWindow window(args.path, args.width, args.height, args.fullscreen);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
    return EXIT_SUCCESS;
}
