#include "vk.hpp"
#include "SDL.h"
#include <random>

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

    void setFileName(const std::string &filename) {
        this->filename = filename;
    }
    
    void initVulkan() override {
        mx::VKWindow::initVulkan();
        logoSprite = createSprite(filename);
    }
    
    void proc() override {
        if (logoSprite) {
            logoSprite->drawSpriteRect(0, 0, getWidth(), getHeight());
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> xDist(0, 1920);
            std::uniform_int_distribution<> yDist(0, 1080);
            std::uniform_int_distribution<> wDist(20, 100);
            std::uniform_int_distribution<> hDist(20, 100);
            spriteData.resize(5000);
            for(auto& sprite : spriteData) {
                sprite.x = xDist(gen);
                sprite.y = yDist(gen);
                sprite.w = wDist(gen);
                sprite.h = hDist(gen);
            }
            for(const auto& sprite : spriteData) {
                logoSprite->drawSpriteRect(sprite.x, sprite.y, sprite.w, sprite.h);
            }
        }

        printText("Hello World", 50, 50, {255, 255, 255, 255});
    }
    void event(SDL_Event& e) override {
        if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
            quit();
        }
    }
private:
    mx::VKSprite* logoSprite = nullptr;
    struct SpriteData {
        int x, y, w, h;
    };
    std::vector<SpriteData> spriteData;
    std::string filename;
};

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
        SkeletonWindow window(args.path, args.width, args.height, args.fullscreen);
        window.setFileName(args.filename);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
    return EXIT_SUCCESS;
}
