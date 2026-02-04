#include "vk.hpp"
#include "SDL.h"
#include <random>
#include <cmath>
#include "argz.hpp"


class MasterPieceWindow : public mx::VKWindow {
public:
    MasterPieceWindow(const std::string& path, int wx, int wy, bool full) 
        : mx::VKWindow("-[ Vulkan Point Sprites - Glowing Light Orbs ]-", wx, wy, full) {
        setPath(path);
    }
    virtual ~MasterPieceWindow() {}

    void initVulkan() override {
        mx::VKWindow::initVulkan();
        initGfx();
    }

    
    void initGfx() {

    }

    void proc() override {
        printText("-[ MasterPiece2D ]-", 20, 20, {255, 255, 255, 255});
    }
    
    void event(SDL_Event& e) override {
        if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
            quit();
            return;
        }
    }
    
private:

};

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
        MasterPieceWindow window(args.path, args.width, args.height, args.fullscreen);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
    return EXIT_SUCCESS;
}
