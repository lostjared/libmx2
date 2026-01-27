#include "vk.hpp"
#include"SDL.h"
#include <format>

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

        if(e.type == SDL_QUIT) {
            quit();
            return;
        }

        if(e.type == SDL_KEYDOWN) {
            switch(e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    quit();
                    break;
                case SDLK_UP:
                    cameraDistance -= 0.5f;
                    if(cameraDistance < 0.5f) cameraDistance = 0.5f;
                    break;
                case SDLK_DOWN:
                    cameraDistance += 0.5f;
                    if(cameraDistance > 50.0f) cameraDistance = 50.0f;
                    break;
            }
        }
    }
    virtual void proc() override {
        SDL_Color white {255, 255, 255, 255};
        SDL_Color green {0, 255, 0, 255};
        SDL_Color red {255, 64, 64, 255};
        SDL_Color yellow {255, 255, 0, 255};
        
        static uint64_t frameCount = 0;
        static Uint64 lastTime = SDL_GetPerformanceCounter();
        static double fps = 0.0;
        static int fpsUpdateCounter = 0;
        
        ++frameCount;
        ++fpsUpdateCounter;
        
        Uint64 currentTime = SDL_GetPerformanceCounter();
        double elapsed = (currentTime - lastTime) / (double)SDL_GetPerformanceFrequency();
        
        if (fpsUpdateCounter >= 10) {
            fps = fpsUpdateCounter / elapsed;
            lastTime = currentTime;
            fpsUpdateCounter = 0;
        }
        std::string fpsText = std::format("FPS: {:.1f}", fps);
        printText("Vulkan Text Rendering Example", 50, 50, white);
        printText("This text is rendered with Vulkan", 50, 100, green);
        printText("Frame: " + std::to_string(frameCount), 50, 150, red);
        printText(fpsText, 50, 200, yellow);
        printText("Press ESC to exit", 50, 250, white);
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