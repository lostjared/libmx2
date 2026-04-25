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
#include "config.h"
#ifndef WITH_MOLTEN
#include "volk.h"
#include <SDL_vulkan.h>
#else
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#endif
#include "exception.hpp"
#include "util.hpp"

#include "vk_sprite.hpp"
#include "vk_text.hpp"
#include <SDL_ttf.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <format>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef VK_CHECK_RESULT
#define VK_CHECK_RESULT(f)                                                                                                              \
    {                                                                                                                                   \
        VkResult res = (f);                                                                                                             \
        if (res != VK_SUCCESS) {                                                                                                        \
            throw mx::Exception(std::format("Fatal : VkResult is \"{}\" in {} at line {}", static_cast<int>(res), __FILE__, __LINE__)); \
        }                                                                                                                               \
    }
#endif

namespace mx {

    class VKAbstractModel;

    /**
     * @struct Vertex
     * @brief Per-vertex data sent to the Vulkan vertex buffer.
     */
    struct Vertex {
        float pos[3];      ///< 3-D position (x, y, z).
        float texCoord[2]; ///< UV texture coordinates.
        float normal[3];   ///< Surface normal vector.
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
        VkSurfaceCapabilitiesKHR capabilities;      ///< Surface capability limits.
        std::vector<VkSurfaceFormatKHR> formats;    ///< Supported colour formats.
        std::vector<VkPresentModeKHR> presentModes; ///< Supported presentation modes.
    };

    /**
     * @struct UniformBufferObject
     * @brief Standard UBO layout uploaded to shaders each frame.
     *
     * Provides model/view/projection matrices plus application-defined
     * parameters and a colour tint.
     */
    struct UniformBufferObject {
        alignas(16) glm::mat4 model;  ///< Model transform matrix.
        alignas(16) glm::mat4 view;   ///< View (camera) matrix.
        alignas(16) glm::mat4 proj;   ///< Projection matrix.
        alignas(16) glm::vec4 params; ///< Application-defined shader parameters.
        alignas(16) glm::vec4 color;  ///< Global colour tint.
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
        virtual ~VKWindow() {}
        VKWindow(const VKWindow &) = delete;
        VKWindow &operator=(const VKWindow &) = delete;
        VKWindow(VKWindow &&) = delete;
        VKWindow &operator=(VKWindow &&) = delete;

        friend class VKAbstractModel;

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
        VkShaderModule createShaderModule(const std::vector<char> &code);

        int w = 0;   ///< Current window width.
        int h = 0;   ///< Current window height.
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
        void updateTexture(SDL_Surface *newSurface);

        /**
         * @brief Replace the primary texture from a raw pixel buffer.
         * @param pixels    Pointer to RGBA data.
         * @param imageSize Size of the data in bytes.
         */
        void updateTexture(void *pixels, VkDeviceSize imageSize);

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
        bool getTextDimensions(const std::string &text, int &width, int &height);

        /**
         * @brief Create a sprite from a PNG file.
         * @param pngPath              Path to the PNG image.
         * @param vertexShader         Vertex shader file path.
         * @param fragmentShaderPath   Fragment shader file path (empty = default).
         * @return Pointer to the created VKSprite (owned by this window).
         */
        VKSprite *createSprite(const std::string &pngPath, const std::string &vertexShader, const std::string &fragmentShaderPath = "");

        /**
         * @brief Create a sprite from an SDL_Surface.
         * @param surface            Source surface.
         * @param vertexShader       Vertex shader file path.
         * @param fragmentShaderPath Fragment shader file path.
         * @return Pointer to the created VKSprite.
         */
        VKSprite *createSprite(SDL_Surface *surface, const std::string &vertexShader, const std::string &fragmentShaderPath = "");

        /**
         * @brief Create a blank sprite of the given dimensions.
         * @param width              Pixel width.
         * @param height             Pixel height.
         * @param vertexShader       Vertex shader file path.
         * @param fragmentShaderPath Fragment shader file path.
         * @return Pointer to the created VKSprite.
         */
        VKSprite *createSprite(int width, int height, const std::string &vertexShader = "", const std::string &fragmentShaderPath = "");

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
        /** @return Vulkan physical device handle. */
        VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
        /** @return Vulkan logical device handle. */
        VkDevice getDevice() const { return device; }
        /** @return Vulkan graphics queue handle. */
        VkQueue getGraphicsQueue() const { return graphicsQueue; }
        /** @return Vulkan command pool handle. */
        VkCommandPool getCommandPool() const { return commandPool; }

      protected:
        bool active = true;                               ///< @c true while the event loop should keep running.
        bool inputControllersInitialized = false;         ///< Whether joystick/controller input is enabled.
        bool enableValidation = true;                     ///< Enable Vulkan validation layers.
        std::string font = "font.ttf";                    ///< Path to the current TTF font.
        int font_size = 24;                               ///< Current font point size.
        VkInstance instance = VK_NULL_HANDLE;             ///< Vulkan instance.
        VkSurfaceKHR surface = VK_NULL_HANDLE;            ///< Window surface.
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; ///< Selected GPU.
        VkDevice device = VK_NULL_HANDLE;                 ///< Logical device.
        VkQueue graphicsQueue = VK_NULL_HANDLE;           ///< Graphics submission queue.
        VkQueue presentQueue = VK_NULL_HANDLE;            ///< Presentation queue.
        VkDescriptorSetLayout descriptorSetLayout;        ///< Main descriptor set layout.

        VkSwapchainKHR swapChain = VK_NULL_HANDLE;        ///< Active swapchain.
        std::vector<VkImage> swapChainImages;             ///< Swapchain image handles.
        VkFormat swapChainImageFormat;                    ///< Colour format of swapchain images.
        VkExtent2D swapChainExtent;                       ///< Swapchain pixel dimensions.
        std::vector<VkImageView> swapChainImageViews;     ///< Image views into swapchain images.
        std::vector<VkBuffer> uniformBuffers;             ///< Per-frame uniform buffers.
        std::vector<VkDeviceMemory> uniformBuffersMemory; ///< Memory for per-frame UBOs.
        std::vector<void *> uniformBuffersMapped;         ///< Persistently mapped UBO pointers.

        VkRenderPass renderPass = VK_NULL_HANDLE;                ///< Main render pass.
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;        ///< Pipeline layout for the main pass.
        VkPipeline graphicsPipeline = VK_NULL_HANDLE;            ///< Filled-polygon graphics pipeline.
        VkPipeline graphicsPipelineMatrix = VK_NULL_HANDLE;      ///< 3-D matrix-enabled graphics pipeline.
        VkPolygonMode currentPolygonMode = VK_POLYGON_MODE_FILL; ///< Current polygon fill mode.
        bool useWireFrame = false;                               ///< Wireframe rendering toggle.
        bool supportsWireFrame = false;                          ///< Whether the device supports wireframe.
        bool externalMemoryFdEnabled = false;                    ///< True when VK_KHR_external_memory_fd is enabled on device.
        std::vector<VkFramebuffer> swapChainFramebuffers;        ///< Framebuffers for each swapchain image.

        VkCommandPool commandPool = VK_NULL_HANDLE;  ///< Command pool for graphics commands.
        std::vector<VkCommandBuffer> commandBuffers; ///< Per-frame command buffers.

        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE; ///< Signals when a swapchain image is acquired.
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE; ///< Signals when rendering is complete.
        VkFence inFlightFence = VK_NULL_HANDLE;               ///< CPU–GPU frame synchronisation fence.

        VkBuffer vertexBuffer = VK_NULL_HANDLE;             ///< Full-screen quad vertex buffer.
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE; ///< Memory for the vertex buffer.
        VkBuffer indexBuffer = VK_NULL_HANDLE;              ///< Full-screen quad index buffer.
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;  ///< Memory for the index buffer.
        uint32_t indexCount = 0;                            ///< Number of indices in the quad.

        VkImage textureImage = VK_NULL_HANDLE;                        ///< Primary texture image.
        VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;           ///< Memory for the primary texture.
        VkImageView textureImageView = VK_NULL_HANDLE;                ///< View into the primary texture.
        VkImageLayout textureImageLayout = VK_IMAGE_LAYOUT_UNDEFINED; ///< Current layout of the primary texture.
        VkSampler textureSampler = VK_NULL_HANDLE;                    ///< Sampler for the primary texture.

        VkDescriptorPool descriptorPool = VK_NULL_HANDLE; ///< Main descriptor pool.
        std::vector<VkDescriptorSet> descriptorSets;      ///< Per-frame descriptor sets.

        VkImage depthImage = VK_NULL_HANDLE;              ///< Depth/stencil image.
        VkDeviceMemory depthImageMemory = VK_NULL_HANDLE; ///< Memory for the depth image.
        VkImageView depthImageView = VK_NULL_HANDLE;      ///< View into the depth image.
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;      ///< Format used for the depth buffer.

        float cameraDistance = 3.0f; ///< Default camera distance from origin.

        std::unique_ptr<VKText> textRenderer;                           ///< Text rendering subsystem.
        VkPipeline textPipeline = VK_NULL_HANDLE;                       ///< Pipeline for text rendering.
        VkPipelineLayout textPipelineLayout = VK_NULL_HANDLE;           ///< Pipeline layout for text rendering.
        VkDescriptorPool textDescriptorPool = VK_NULL_HANDLE;           ///< Descriptor pool for text.
        VkDescriptorSetLayout textDescriptorSetLayout = VK_NULL_HANDLE; ///< Descriptor layout for text.

        std::vector<std::unique_ptr<VKSprite>> sprites;                   ///< Owned sprite instances.
        VkPipeline spritePipeline = VK_NULL_HANDLE;                       ///< Pipeline for sprite rendering.
        VkPipelineLayout spritePipelineLayout = VK_NULL_HANDLE;           ///< Pipeline layout for sprites.
        VkDescriptorPool spriteDescriptorPool = VK_NULL_HANDLE;           ///< Descriptor pool for sprites.
        VkDescriptorSetLayout spriteDescriptorSetLayout = VK_NULL_HANDLE; ///< Descriptor layout for sprites.
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;         ///< Vulkan debug messenger (validation layers).
        uint32_t width, height;                                           ///< Internal width/height tracking.
        /** @brief Create the main descriptor set layout (UBO + sampler). */
        void createDescriptorSetLayout();
        /** @brief Create depth buffer image, memory, and view. */
        void createDepthResources();
        /**
         * @brief Find a supported VkFormat from the candidate list.
         * @param candidates Ordered list of preferred formats.
         * @param tiling     Required tiling mode.
         * @param features   Required format feature flags.
         * @return First candidate that satisfies the requirements.
         */
        VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        /** @brief Select a depth buffer format supported by the physical device. */
        VkFormat findDepthFormat();
        SDL_Window *window = nullptr;       ///< SDL window handle.
        SDL_Surface *surface_img = nullptr; ///< Cached SDL surface for texture uploads.
        /** @brief Create the Vulkan instance and enable extensions/layers. */
        void createInstance();
        /** @brief Create the Vulkan surface from the SDL window. */
        void createSurface();
        /** @brief Enumerate GPUs and select the most suitable physical device. */
        void pickPhysicalDevice();
        /** @brief Create the logical device and retrieve queue handles. */
        void createLogicalDevice();
        /** @brief Allocate per-frame uniform buffers and map them persistently. */
        void createUniformBuffers();
        /** @brief Create (or recreate) the swapchain and associated images (overridable). */
        virtual void createSwapChain();
        /** @brief Create image views for every swapchain image. */
        void createImageViews();
        /** @brief Create the main render pass with colour and depth attachments. */
        void createRenderPass();
        /** @brief Create framebuffers for each swapchain image view. */
        void createFramebuffers();
        /** @brief Create the graphics command pool. */
        void createCommandPool();
        /** @brief Allocate per-frame command buffers from the command pool. */
        void createCommandBuffers();
        /** @brief Create semaphores and fences for frame synchronisation. */
        void createSyncObjects();
        /** @brief Destroy swapchain-dependent resources before recreation (overridable). */
        virtual void cleanupSwapChain();
        /**
         * @brief Find queue families supporting graphics and presentation.
         * @param device Physical device to query.
         * @return Populated QueueFamilyIndices.
         */
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        /**
         * @brief Choose the best surface format from available options.
         * @param availableFormats Candidate formats.
         * @return Selected surface format.
         */
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
        /**
         * @brief Choose the best present mode (prefers mailbox, falls back to FIFO).
         * @param availablePresentModes Candidate modes.
         * @return Selected presentation mode.
         */
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
        /**
         * @brief Determine the swapchain image extent from surface capabilities.
         * @param capabilities Current surface capabilities.
         * @return Chosen extent in pixels.
         */
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
        /**
         * @brief Check whether a physical device meets all application requirements.
         * @param device Candidate physical device.
         * @return @c true if the device is suitable.
         */
        bool isDeviceSuitable(VkPhysicalDevice device);
        /**
         * @brief Query swapchain support details for a physical device.
         * @param device Physical device to query.
         * @return Populated SwapChainSupportDetails.
         */
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        /**
         * @brief Allocate a Vulkan buffer with the given usage and memory properties.
         * @param size       Buffer size in bytes.
         * @param usage      Vulkan buffer usage flags.
         * @param properties Required memory property flags.
         * @param buffer     Output buffer handle.
         * @param bufferMemory Output device memory handle.
         */
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
        /**
         * @brief Find a device memory type matching the filter and property flags.
         * @param typeFilter Bit mask of acceptable memory types.
         * @param properties Required memory property flags.
         * @return Memory type index.
         */
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        /**
         * @brief Copy data between two Vulkan buffers via a one-shot command.
         * @param srcBuffer Source buffer.
         * @param dstBuffer Destination buffer.
         * @param size      Number of bytes to copy.
         */
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        /**
         * @brief Create a Vulkan texture image from an SDL surface.
         * @param surfacex Source surface (RGBA).
         */
        void createTextureImage(SDL_Surface *surfacex);
        /**
         * @brief Create a Vulkan image with the specified parameters.
         * @param width      Image width in pixels.
         * @param height     Image height in pixels.
         * @param format     Pixel format.
         * @param tiling     Tiling arrangement.
         * @param usage      Image usage flags.
         * @param properties Memory property flags.
         * @param image      Output image handle.
         * @param imageMemory Output device memory handle.
         */
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);
        /**
         * @brief Transition an image between two layouts using a pipeline barrier.
         * @param image     Target image.
         * @param format    Image format (used for aspect flag selection).
         * @param oldLayout Current layout.
         * @param newLayout Desired layout.
         */
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        /**
         * @brief Copy a staging buffer into a Vulkan image.
         * @param buffer Source buffer.
         * @param image  Destination image.
         * @param width  Image width.
         * @param height Image height.
         */
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        /** @brief Begin a one-shot command buffer for transfer operations. */
        VkCommandBuffer beginSingleTimeCommands();
        /** @brief End and submit a one-shot command buffer, then wait for completion. */
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        /** @brief Create the image view for the primary texture. */
        void createTextureImageView();
        /**
         * @brief Create an image view with specified format and aspect flags.
         * @param image       Source image.
         * @param format      View format.
         * @param aspectFlags Aspect mask (colour, depth, etc.).
         * @return Created VkImageView.
         */
        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
        /** @brief Create the texture sampler with default filter/addressing modes. */
        void createTextureSampler();
        /** @brief Create the main descriptor pool. */
        void createDescriptorPool();
        /** @brief Allocate main descriptor sets from the pool. */
        void createDescriptorSets();
        /** @brief Write updated bindings into existing descriptor sets. */
        void updateDescriptorSets();
        /** @brief Tear down and rebuild the swapchain and all dependent resources. */
        void recreateSwapChain();
        /**
         * @brief Allocate a blank texture image of the given dimensions.
         * @param w Width in pixels.
         * @param h Height in pixels.
         */
        void setupTextureImage(uint32_t w, uint32_t h);
        /** @brief Create the descriptor set layout for text rendering. */
        void createTextDescriptorSetLayout();
        /** @brief Create the pipeline for text rendering. */
        void createTextPipeline();
        /** @brief Create the descriptor pool for text rendering. */
        void createTextDescriptorPool();
        /** @brief Create the descriptor set layout for sprite rendering. */
        void createSpriteDescriptorSetLayout();
        /** @brief Create the pipeline for sprite rendering. */
        void createSpritePipeline();
        /** @brief Create the descriptor pool for sprite rendering. */
        void createSpriteDescriptorPool();
    };

} // namespace mx

#endif
