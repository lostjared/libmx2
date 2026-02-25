/**
 * @file gl_cv.hpp
 * @brief OpenCV video-capture integration for the OpenGL backend.
 *
 * MXCapture wraps cv::VideoCapture and renders each captured frame into
 * a gl::GLSprite using a user-supplied GLSL shader pair.
 */
#ifndef __GL_OPENCV__H_
#define __GL_OPENCV__H_

#include<opencv2/opencv.hpp>
#include"gl.hpp"
#include<memory>

namespace mx {

/**
 * @class MXCapture
 * @brief OpenCV video capture source that renders via an OpenGL sprite.
 *
 * Opens a video file or camera device, reads frames frame-by-frame, and
 * uploads each frame to a gl::GLSprite texture for display inside a
 * gl::GLWindow with a custom GLSL shader.
 */
    class MXCapture {
    public:
        /** @brief Default constructor. */
        MXCapture() = default;
        /** @brief Destructor. */
        ~MXCapture() = default;
        MXCapture &operator=(const MXCapture &) = delete;
        MXCapture &operator=(MXCapture &&) = delete;
        MXCapture(const MXCapture &) = delete;
        MXCapture(MXCapture &&) = delete;

        /**
         * @brief Open a video file for capture.
         * @param filename Path to the video file.
         * @return @c true on success.
         */
        bool open(const std::string &filename);

        /**
         * @brief Open a camera device for capture.
         * @param id   Camera device index.
         * @param mode Optional backend hint (0 = auto).
         * @return @c true on success.
         */
        bool open(int id, int mode=0);

        /** @brief Close the video source. */
        void close();

        /**
         * @brief Check whether the capture source is open.
         * @return @c true if the VideoCapture is opened.
         */
        bool is_open() const { return cap.isOpened(); }

        /**
         * @brief Create the backing sprite/texture for displaying frames.
         * @param window OpenGL window context.
         * @param width  Frame display width.
         * @param height Frame display height.
         * @param vert   Path to the vertex shader GLSL file.
         * @param frag   Path to the fragment shader GLSL file.
         * @return @c true on success.
         */
        bool createImage(gl::GLWindow *window, size_t width, size_t height, const std::string &vert, const std::string &frag);

        /**
         * @brief Access the internal sprite.
         * @return Pointer to the backing gl::GLSprite.
         */
        gl::GLSprite *getSprite() { return &sprite; }

        /**
         * @brief Draw the most recently captured frame.
         * @param x      Destination X position.
         * @param y      Destination Y position.
         * @param width  Display width.
         * @param height Display height.
         */
        void draw(int x, int y, int width, int height);

        /**
         * @brief Draw using the sprite's native dimensions.
         * @param x Destination X position.
         * @param y Destination Y position.
         */
        void draw(int x, int y);

        /**
         * @brief Replace the shader pair without reopening the source.
         * @param vert New vertex shader path.
         * @param frag New fragment shader path.
         * @return @c true on success.
         */
        bool reload(const std::string &vert, const std::string &frag);

        /**
         * @brief Grab and decode the next frame from the source.
         * @return @c true if a new frame was available.
         */
        bool read();

        /**
         * @brief Grab and decode the next frame into a cv::Mat.
         * @param frame Output matrix.
         * @return @c true if a new frame was available.
         */
        bool read(cv::Mat &frame);

        /**
         * @brief Set a VideoCapture property.
         * @param option cv::VideoCaptureProperties constant.
         * @param value  Desired value.
         */
        void set(unsigned int option, double value);

        /**
         * @brief Query a VideoCapture property.
         * @param option cv::VideoCaptureProperties constant.
         * @return Current property value.
         */
        double get(unsigned int option);
    private:
        gl::GLSprite sprite;                        ///< Rendering sprite.
        std::unique_ptr<gl::ShaderProgram> shader;  ///< Active shader program.
        gl::GLWindow *window = nullptr;             ///< Parent GL window.
        cv::VideoCapture cap;                       ///< OpenCV capture device.
        cv::Mat frame;                              ///< Most recent decoded frame.
    };
}

#endif
