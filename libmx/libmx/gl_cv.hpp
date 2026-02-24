#ifndef __GL_OPENCV__H_
#define __GL_OPENCV__H_

#include<opencv2/opencv.hpp>
#include"gl.hpp"
#include<memory>

namespace mx {
    class MXCapture {
    public:
        MXCapture() = default;
        ~MXCapture() = default;
        MXCapture &operator=(const MXCapture &) = delete;
        MXCapture &operator=(MXCapture &&) = delete;
        MXCapture(const MXCapture &) = delete;
        MXCapture(MXCapture &&) = delete;
        bool open(const std::string &filename);
        bool open(int id, int mode=0);
        void close();
        bool is_open() const { return cap.isOpened(); }
        bool createImage(gl::GLWindow *window, size_t width, size_t height, const std::string &vert, const std::string &frag);
        gl::GLSprite *getSprite() { return &sprite; }
        void draw(int x, int y, int width, int height);
        void draw(int x, int y);
        bool reload(const std::string &vert, const std::string &frag);
        bool read();
        bool read(cv::Mat &frame);
        void set(unsigned int option, double value);
        double get(unsigned int option);
    private:
        gl::GLSprite sprite;
        std::unique_ptr<gl::ShaderProgram> shader;
        gl::GLWindow *window = nullptr;
        cv::VideoCapture cap;
        cv::Mat frame;
    };
}

#endif
