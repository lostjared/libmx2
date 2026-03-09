/**
 * @file gl_cv.cpp
 * @brief Implementation of mx::MXCapture (OpenGL + OpenCV variant).
 */
#include"config.h"
#include"gl_cv.hpp"
#include<filesystem>

namespace mx {

    bool MXCapture::open(const std::string &filename) {
        return cap.open(filename);
    }

    bool MXCapture::open(int id, int mode) {
        if(mode == 0)
            mode = cv::CAP_V4L2;
        if(cap.open(id, mode)) {
            cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M','J','P','G'));
            return true;
        }
        return false;
    }
    
    void MXCapture::close() {
        cap.release();
    }

    bool MXCapture::createImage(gl::GLWindow *window, size_t width, size_t height, const std::string &vert, const std::string &frag) {
        shader.reset (new gl::ShaderProgram());
        if(!shader->loadProgram(vert, frag)) {
            throw mx::Exception(">> MXCapture: Could not load shader.\n");
        }
        this->window = window;
        sprite.initSize(window->w, window->h);
        sprite.setShader(shader.get());
        cv::Mat mat = cv::Mat::zeros(width, height, CV_8UC4);
        GLuint texture = gl::createTexture(mat.ptr(), width, height);
        sprite.initWithTexture(shader.get(), texture, 0.0f, 0.0f,
            static_cast<int>(width), static_cast<int>(height));
        return true;
    }

    bool MXCapture::reload(const std::string &vert, const std::string &frag) {
        shader.reset(new gl::ShaderProgram());
        if(!shader->loadProgram(vert, frag)) {
            throw mx::Exception(">> MXCapture: could not load shader: " + vert + "/" + frag);
        }
        return false;
    }

    bool MXCapture::read(cv::Mat &frame) {
        return cap.read(frame);
    }

    bool MXCapture::read() {
        if(!cap.read(frame)) {
            return false;
        }
        cv::Mat rgba;
        cv::cvtColor(frame, rgba, cv::COLOR_BGR2RGBA);
        cv::flip(rgba, rgba, 0);
        sprite.updateTexture(rgba.ptr(),rgba.cols, rgba.rows);
        return true;
    }

    void MXCapture::draw(int x, int y, int width, int height) {
        if (window) {
            sprite.initSize(static_cast<float>(window->w), static_cast<float>(window->h));
        }
        sprite.draw(x, y, width, height);
    }

    void MXCapture::draw(int x, int y) {
        if (window) {
            sprite.initSize(static_cast<float>(window->w), static_cast<float>(window->h));
        }
        sprite.draw(x, y);
    }
        
    void MXCapture::set(unsigned int option, double value) {
        cap.set(option, value);
    }
        
    double MXCapture::get(unsigned int option) {
        return cap.get(option);
    }

}