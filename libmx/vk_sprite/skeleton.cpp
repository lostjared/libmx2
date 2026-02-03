#include "vk.hpp"
#include "SDL.h"
#include <random>
#include "argz.hpp"
class SkeletonWindow : public mx::VKWindow {
public:
    SkeletonWindow(const std::string& path, int wx, int wy, bool full) 
        : mx::VKWindow("-[ Vulkan Skeleton / Random Sprites ]-", wx, wy, full) {
        setPath(path);
    }
    virtual ~SkeletonWindow() {}

    void setFileName(const std::string &filename) {
        this->filename = filename;
    }
    
    void initVulkan() override {
        const char *szBlocks[] = {"red1.png", "red2.png", "red3.png", "green1.png", "green2.png", "green3.png", "blue1.png", "blue2.png", "blue3.png", nullptr };
        mx::VKWindow::initVulkan();
        for(int i = 0; szBlocks[i] != nullptr; ++i) {
            blocks[i] = createSprite(util.path + "/data/img/" + szBlocks[i]);
        }
    }
    
    void proc() override {
        int grid_w = getWidth() / 32;
        int grid_h = getHeight() /  16;
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 8);
        for(int i = 0; i < grid_w; ++i) {
            for(int z = 0; z < grid_h; ++z) {
                int blockIndex = dis(gen);
                if(blocks[blockIndex]) {
                    blocks[blockIndex]->drawSpriteRect(i*32, z*16, 32, 16);
                }
            }
        }
        printText("-[ Hello World Random Sprites ]-", 50, 50, {255, 255, 255, 255});
    }
    void event(SDL_Event& e) override {
        if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
            quit();
        }
    }
private:
    mx::VKSprite* blocks[9];
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
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
    return EXIT_SUCCESS;
}
