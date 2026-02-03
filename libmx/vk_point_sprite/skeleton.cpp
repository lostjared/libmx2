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
    
    static constexpr int block_w = 8;
    static constexpr int block_h = 8;

    void proc() override {
        
        int grid_w = getWidth() / block_w;
        int grid_h = getHeight() /  block_h;
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 8);
        float currentTime = static_cast<float>(SDL_GetTicks()) / 1000.0f;
        for(int i = 0; i < grid_w; ++i) {
            for(int z = 0; z < grid_h; ++z) {
                int blockIndex = dis(gen);
                float px = i * block_w;
                float py = z * block_h;
                float spriteTime = currentTime + (z * 0.02f); 
                spriteTime += (rand() % 100 / 1000.0f); 
                blocks[blockIndex]->setShaderParams(0.0f, 0.0f, 1.0f, spriteTime);
                blocks[blockIndex]->drawSpriteRect(px, py, block_w, block_h);
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
