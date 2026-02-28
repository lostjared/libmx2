/**
 * @file vk_cv.hpp
 * @brief OpenCV video-capture integration for the Vulkan backend.
 *
 * mx::MXCapture (Vulkan variant) wraps cv::VideoCapture and feeds decoded
 * frames into a VKSprite for display within a VKWindow.
 */
#ifndef __VK_OPENCV__H_
#define __VK_OPENCV__H_

#include<opencv2/opencv.hpp>
#include"vk.hpp"
#include"vk_sprite.hpp"

namespace mx {

/**
 * @class MXCapture
 * @ingroup mxvk_cv_module
 * @brief Vulkan OpenCV video capture source.
 *
 * Opens a video file or camera, decodes frames, and uploads them to a
 * VKSprite texture for Vulkan-accelerated rendering inside a VKWindow.
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
         * @brief Open a video file.
         * @param filename Path to the video file.
         * @return @c true on success.
         */
        bool open(const std::string &filename);

        /**
         * @brief Open a camera device.
         * @param id   Camera index.
         * @param mode Backend hint (0 = auto).
         * @return @c true on success.
         */
        bool open(int id, int mode=0);

        /** @brief Close the capture device. */
        void close();

        /**
         * @brief Check whether the capture device is open.
         * @return @c true if the VideoCapture is opened.
         */
        bool is_open() const { return cap.isOpened(); }

        /**
         * @brief Allocate the VKSprite and upload the initial texture.
         * @param window Parent VKWindow.
         * @param width  Display width.
         * @param height Display height.
         * @param vert   Vertex shader path.
         * @param frag   Fragment shader path.
         * @return @c true on success.
         */
        bool createImage(VKWindow *window, size_t width, size_t height, const std::string &vert, const std::string &frag);

        /**
         * @brief Access the backing sprite.
         * @return Pointer to the VKSprite.
         */
        VKSprite *getSprite() { return sprite; }

        /**
         * @brief Draw the current frame at a specified position and size.
         * @param x Width destination X.
         * @param y Width destination Y.
         * @param width  Display width.
         * @param height Display height.
         */
        void draw(int x, int y, int width, int height);

        /**
         * @brief Draw using the sprite's native dimensions.
         * @param x Destination X.
         * @param y Destination Y.
         */
        void draw(int x, int y);

        /**
         * @brief Replace the vertex/fragment shaders.
         * @param width  Display width.
         * @param height Display height.
         * @param vert   New vertex shader path.
         * @param frag   New fragment shader path.
         * @return @c true on success.
         */
        bool reload(size_t width, size_t height, const std::string &vert, const std::string &frag);

        /**
         * @brief Capture and upload the next video frame.
         * @return @c true if a frame was available.
         */
        bool read();

        /**
         * @brief Capture the next frame into a cv::Mat.
         * @param frame Output matrix.
         * @return @c true if a frame was available.
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
         * @return Current value.
         */
        double get(unsigned int option);
    private:
        VKSprite *sprite = nullptr;    ///< Backing Vulkan sprite.
        cv::VideoCapture cap;          ///< OpenCV capture device.
        cv::Mat frame;                 ///< Most recent decoded frame.
    };
}

#endif
