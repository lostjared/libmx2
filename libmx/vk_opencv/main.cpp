#include "vk.hpp"
#include "SDL.h"
#include "argz.hpp"
#include<opencv2/opencv.hpp>


class ExampleWindow : public mx::VKWindow {
public:
    ExampleWindow(const std::string& path, int wx, int wy, bool full)
        : mx::VKWindow("-[ Vulkan Example / Sprite ]-", wx, wy, full) {
        setPath(path);
    }
    void initVulkan() override {
        mx::VKWindow::initVulkan();
    }

    void openCamera(int index, int width, int height) {
        cap.open(index);
        if(!cap.isOpened()) {
            throw mx::Exception("Error could not open camera at index: " + std::to_string(index));
        }
        cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
        camera_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        camera_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        resizeWindow(camera_width, camera_height);
        sprite = createSprite(camera_width, camera_height, util.path + "/data/sprite_vert.spv", util.path + "/data/sprite_frag.spv");
    }
    
    void openFile(const std::string &filename) {
        cap.open(filename);
        if(!cap.isOpened()) {
            throw mx::Exception("Error could not open file: " + filename);
        }
        camera_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        camera_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        //resizeWindow(camera_width, camera_height);
        sprite = createSprite(camera_width, camera_height, util.path + "/data/sprite_vert.spv", util.path + "/data/sprite_frag.spv");
    }
    void proc() override {
        if (!sprite) {
            return;
        }
        cv::Mat frame;
        if(!cap.read(frame)) {
            quit();
            return;
        }

        cv::Mat rgba;
        cv::cvtColor(frame, rgba, cv::COLOR_BGR2RGBA);
        sprite->updateTexture(rgba.data, rgba.cols, rgba.rows, rgba.step);
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
    cv::VideoCapture cap;
    int camera_width = 0;
    int camera_height = 0;
};

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
        ExampleWindow window(args.path, args.width, args.height, args.fullscreen);
        window.initVulkan();
        //window.openCamera(0, args.width, args.height);
        window.openFile(args.filename);
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
        return EXIT_FAILURE;
    } catch(std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return EXIT_SUCCESS;
}
