/**
 * @file vk.hpp
 * @brief Vulkan application window and supporting structs for libmx.
 *
 * Provides VKWindow, the primary Vulkan rendering window, along with
 * supporting data structures (Vertex, QueueFamilyIndices,
 * SwapChainSupportDetails, UniformBufferObject).  Sprite and text
 * rendering are delegated to VKSprite and VKText respectively.
 */
#ifndef _VK_MX_H__
#define _VK_MX_H__
#include"config.h"
#ifndef WITH_MOLTEN
#include "volk.h"
#include <SDL_vulkan.h>
#else
#include <SDL_vulkan.h>
#include<vulkan/vulkan.h>
#endif
#include"exception.hpp"
#include "util.hpp"

#include "vk_text.hpp"
#include "vk_sprite.hpp"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <fstream>
#include <algorithm>
#include <array>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <utility>
#include <SDL_ttf.h>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define VK_CHECK_RESULT(f) { \
    VkResult res = (f); \
    if (res != VK_SUCCESS) { \
        throw mx::Exception("Fatal : VkResult is \"" + std::to_string(res) + "\" in " + __FILE__ + " at line " + std::to_string(__LINE__)); \
    } \
}

namespace mx {

/**
 * @struct Vertex
 * @brief Per-vertex data sent to the Vulkan vertex buffer.
 */
    struct Vertex {
        float pos[3];       ///< 3-D position (x, y, z).
        float texCoord[2];  ///< UV texture coordinates.
        float normal[3];    ///< Surface normal vector.
    };

/**
 * @struct QueueFamilyIndices
 * @brief Tracks Vulkan queue family assignments needed to create a device.
 */
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily; ///< Index of the graphics queue family.
        std::optional<uint32_t> presentFamily;  ///< Index of the presentation queue family.

        /**
         * @brief Check whether both required families have been found.
         * @return @c true if graphics and present families are both assigned.
         */
        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

/**
 * @struct SwapChainSupportDetails
 * @brief Swapchain capabilities and format/mode candidates for a physical device.
 */
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;          ///< Surface capability limits.
        std::vector<VkSurfaceFormatKHR> formats;        ///< Supported colour formats.
        std::vector<VkPresentModeKHR> presentModes;     ///< Supported presentation modes.
    };

/**
 * @struct UniformBufferObject
 * @brief Standard UBO layout uploaded to shaders each frame.
 *
 * Provides model/view/projection matrices plus application-defined
 * parameters and a colour tint.
 */
    struct UniformBufferObject {
        alignas(16) glm::mat4 model;        ///< Model transform matrix.
        alignas(16) glm::mat4 view;         ///< View (camera) matrix.
        alignas(16) glm::mat4 proj;         ///< Projection matrix.
        alignas(16) glm::vec4 params;       ///< Application-defined shader parameters.
        alignas(16) glm::vec4 color;        ///< Global colour tint.
    };

/**
 * @class VKWindow
 * @brief Primary Vulkan rendering window and device management class.
 *
 * Encapsulates the full Vulkan initialisation sequence: instance, surface,
 * physical and logical device selection, swap chain creation, render pass,
 * descriptor sets, command buffers, and synchronisation.  Derived classes
 * implement event() and optionally override draw(), initVulkan(), and
 * cleanup() to add application-specific rendering.
 *
 * Built-in support for:
 * - 2-D text rendering via VKText
 * - Sprite rendering via VKSprite
 * - Texture upload (SDL_Surface or raw pixels)
 * - Wireframe toggle
 */
    class VKWindow {
    public:
        VKWindow() = default;

        /**
         * @brief Create and fully initialise the Vulkan window.
         * @param title  Window title string.
         * @param width  Width in pixels.
         * @param height Height in pixels.
         * @param full   Start fullscreen if @c true.
         * @param valid  Enable Vulkan validation layers if @c true.
         */
        VKWindow(const std::string &title, int width, int height, bool full = false, bool valid = true);

        /** @brief Virtual destructor — subclasses should call cleanup(). */
        virtual ~VKWindow() { }
        VKWindow(const VKWindow&) = delete;
        VKWindow& operator=(const VKWindow&) = delete;
        VKWindow(VKWindow&&) = delete;
        VKWindow& operator=(VKWindow&&) = delete;

        /**
         * @brief Open the SDL2 window (called automatically by constructor).
         * @param title  Title string.
         * @param width  Width.
         * @param height Height.
         * @param full   Fullscreen flag.
         */
        void initWindow(const std::string &title, int width, int height, bool full = false);

        /**
         * @brief Set the font used for printText().
         * @param font Path to the TTF font file.
         * @param size Point size.
         */
        void setFont(const std::string &font, const int size = 24);

        /** @brief Initialise all Vulkan objects (overridable). */
        virtual void initVulkan();

        /** @brief Create and upload the vertex buffer. */
        void createVertexBuffer();

        /**
         * @brief Set the asset root directory.
         * @param path Asset directory path.
         */
        void setPath(const std::string &path);

        /** @brief Run the blocking event/render loop. */
        void loop();

        /** @brief Process one event/render iteration (overridable). */
        virtual void proc();

        /** @brief Free all Vulkan resources (overridable). */
        virtual void cleanup();

        /**
         * @brief Handle an SDL event (pure virtual).
         * @param e SDL event to process.
         */
        virtual void event(SDL_Event &e) = 0;

        /**
         * @brief Record rendering commands for the current frame (overridable).
         *
         * The default implementation draws the main textured quad and updates
         * the uniform buffer.
         */
        virtual void draw();

        /** @brief Called when the window is resized (optional override). */
        virtual void onResize() {}

        /**
         * @brief Control whether 3-D geometry is rendered this frame.
         * @return @c true (default) to submit the 3-D render pass.
         */
        virtual bool shouldRender3D() { return true; }

        /** @brief (Re)create the graphics pipeline. */
        void createGraphicsPipeline();

        /**
         * @brief Compile a SPIR-V shader module.
         * @param code Byte vector of SPIR-V code.
         * @return VkShaderModule handle.
         */
        VkShaderModule createShaderModule(const std::vector<char>& code);

        int w = 0; ///< Current window width.
        int h = 0; ///< Current window height.
        mxUtil util; ///< Asset path and utility helpers.

        /** @brief Post a quit event to stop the loop. */
        void quit();

        /**
         * @brief Toggle fullscreen mode.
         * @param full @c true for fullscreen.
         */
        void setFullScreen(const bool full);

        /**
         * @brief Replace the primary texture from an SDL_Surface.
         * @param newSurface Source surface (not consumed).
         */
        void updateTexture(SDL_Surface* newSurface);

        /**
         * @brief Replace the primary texture from a raw pixel buffer.
         * @param pixels    Pointer to RGBA data.
         * @param imageSize Size of the data in bytes.
         */
        void updateTexture(void* pixels, VkDeviceSize imageSize);

        /**
         * @brief Queue a text string for rendering at a screen position.
         * @param text String to display.
         * @param x    X position in pixels.
         * @param y    Y position in pixels.
         * @param col  Text colour.
         */
        void printText(const std::string &text, int x, int y, const SDL_Color &col);

        /** @brief Clear all pending text-render entries. */
        void clearTextQueue();

        /**
         * @brief Measure a text string without rendering it.
         * @param text   String to measure.
         * @param width  Output: pixel width.
         * @param height Output: pixel height.
         * @return @c true if measurement succeeded.
         */
        bool getTextDimensions(const std::string &text, int& width, int& height);

        /**
         * @brief Create a sprite from a PNG file.
         * @param pngPath              Path to the PNG image.
         * @param vertexShader         Vertex shader file path.
         * @param fragmentShaderPath   Fragment shader file path (empty = default).
         * @return Pointer to the created VKSprite (owned by this window).
         */
        VKSprite* createSprite(const std::string &pngPath, const std::string &vertexShader, const std::string &fragmentShaderPath = "");

        /**
         * @brief Create a sprite from an SDL_Surface.
         * @param surface            Source surface.
         * @param vertexShader       Vertex shader file path.
         * @param fragmentShaderPath Fragment shader file path.
         * @return Pointer to the created VKSprite.
         */
        VKSprite* createSprite(SDL_Surface* surface, const std::string &vertexShader, const std::string &fragmentShaderPath = "");

        /**
         * @brief Create a blank sprite of the given dimensions.
         * @param width              Pixel width.
         * @param height             Pixel height.
         * @param vertexShader       Vertex shader file path.
         * @param fragmentShaderPath Fragment shader file path.
         * @return Pointer to the created VKSprite.
         */
        VKSprite* createSprite(int width, int height, const std::string &vertexShader = "", const std::string &fragmentShaderPath = "");

        /** @return Current window width. */
        int getWidth() const { return w; }
        /** @return Current window height. */
        int getHeight() const { return h; }

        /**
         * @brief Resize the SDL window and recreate the swapchain.
         * @param width  New width.
         * @param height New height.
         */
        void resizeWindow(int width, int height);

        /**
         * @brief Enable joystick and game controller subsystems on demand.
         * @return @c true if controller input is ready, @c false on failure.
         */
        bool enableControllerInput();

        /** @return @c true if controller input subsystem was enabled. */
        bool controllerInputEnabled() const { return inputControllersInitialized; }

        /**
         * @brief Enable or disable wireframe rendering.
         * @param enable @c true to draw in wireframe.
         */
        void setWireFrame(bool enable) { useWireFrame = enable; }

        /** @return @c true if wireframe mode is active. */
        bool getWireFrame() const { return useWireFrame; }
    protected:
        bool active = true;
        bool inputControllersInitialized = false;
        bool enableValidation = true;
        std::string font = "font.ttf";
        int font_size = 24;
        VkInstance instance = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout;

        VkSwapchainKHR swapChain = VK_NULL_HANDLE;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void*> uniformBuffersMapped;

        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
        VkPipeline graphicsPipelineMatrix = VK_NULL_HANDLE;
        VkPolygonMode currentPolygonMode = VK_POLYGON_MODE_FILL;
        bool useWireFrame = false;
        bool supportsWireFrame = false;
        std::vector<VkFramebuffer> swapChainFramebuffers;

        VkCommandPool commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> commandBuffers;

        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
        VkFence inFlightFence = VK_NULL_HANDLE;

        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
        uint32_t indexCount = 0;

        VkImage textureImage = VK_NULL_HANDLE;
        VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
        VkImageView textureImageView = VK_NULL_HANDLE;
        VkImageLayout textureImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkSampler textureSampler = VK_NULL_HANDLE;
        
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets;
        
        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
        VkImageView depthImageView = VK_NULL_HANDLE;
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
        
        float cameraDistance = 3.0f;
        
        std::unique_ptr<VKText> textRenderer;
        VkPipeline textPipeline = VK_NULL_HANDLE;
        VkPipelineLayout textPipelineLayout = VK_NULL_HANDLE;
        VkDescriptorPool textDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout textDescriptorSetLayout = VK_NULL_HANDLE;
        
        std::vector<std::unique_ptr<VKSprite>> sprites;
        VkPipeline spritePipeline = VK_NULL_HANDLE;
        VkPipelineLayout spritePipelineLayout = VK_NULL_HANDLE;
        VkDescriptorPool spriteDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout spriteDescriptorSetLayout = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        uint32_t width, height;
        void createDescriptorSetLayout();
        void createDepthResources();
        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        VkFormat findDepthFormat();
        SDL_Window *window = nullptr;
        SDL_Surface *surface_img = nullptr;
        void createInstance();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createUniformBuffers();
        virtual void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();
        virtual void cleanupSwapChain();
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        bool isDeviceSuitable(VkPhysicalDevice device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void createTextureImage(SDL_Surface* surfacex);
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        void createTextureImageView();
        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
        void createTextureSampler();
        void createDescriptorPool();
        void createDescriptorSets();
        void updateDescriptorSets();
        void recreateSwapChain();
        void setupTextureImage(uint32_t w, uint32_t h);
        void createTextDescriptorSetLayout();
        void createTextPipeline();
        void createTextDescriptorPool();
        void createSpriteDescriptorSetLayout();
        void createSpritePipeline();
        void createSpriteDescriptorPool();
    };

}

#endif
