#include"vk_cv.hpp"

namespace mx {

    bool MXCapture::open(const std::string &filename) {
        return cap.open(filename);
    }

    bool MXCapture::open(int id, int mode) {
        if(mode == 0)
            mode = cv::CAP_V4L2;

        return cap.open(id, mode);
    }
    
    void MXCapture::close() {
        cap.release();
    }

    bool MXCapture::createImage(VKWindow *window, size_t width, size_t height, const std::string &vert, const std::string &frag) {
        sprite = window->createSprite(width, height, vert, frag);
        if(sprite) {
            sprite->enableExtendedUBO();
            sprite->rebuildPipeline();
            return true;
        }
        return false;
    }

    bool MXCapture::reload(size_t width, size_t height, const std::string &vert, const std::string &frag) {
        if(sprite) {
            sprite->createEmptySprite(width, height,vert, frag);
            sprite->enableExtendedUBO();
            return true;
        }
        return false;
    }

    bool MXCapture::read() {
        if(!cap.read(frame)) {
            return false;
        }
        cv::Mat rgba;
        cv::cvtColor(frame, rgba, cv::COLOR_BGR2RGBA);
        sprite->updateTexture(rgba.ptr(), rgba.rows, rgba.cols, rgba.step);
        return true;
    }

    void MXCapture::draw(int x, int y, int width, int height) {
        if(sprite)
            sprite->drawSpriteRect(x, y, width, height);
    }

    void MXCapture::draw(int x, int y) {
        if(sprite)
            sprite->drawSprite(x, y);
    }
        
    void MXCapture::set(unsigned int option, double value) {
        cap.set(option, value);
    }
        
    double MXCapture::get(unsigned int option) {
        return cap.get(option);
    }

}