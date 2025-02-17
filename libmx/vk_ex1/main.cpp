#include "vk.hpp"

class MainWindow : public mx::VKWindow {
public:
    MainWindow() : mx::VKWindow("Hello, World with Vulkan", 1024, 768) {}
    virtual ~MainWindow() {}
    virtual void event(SDL_Event &e) override {}
};



int main(int argc, char **argv) {
    try {
        MainWindow window;
        window.loop();   
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
    return 0;
}