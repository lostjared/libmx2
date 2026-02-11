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
    
    struct Vertex {
        float pos[3];
        float texCoord[2];
        float normal[3];
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
        alignas(16) glm::vec4 params;      // x=time, y=unused, z=unused, w=unused
        alignas(16) glm::vec4 color;  
        alignas(16) glm::vec4 playerPos;   // x=posX, y=posZ, z=dirX, w=dirZ
        alignas(16) glm::vec4 playerPlane; // x=planeX, y=planeZ, z=screenW, w=screenH
    };

    class VKWindow {
    public:
        VKWindow() = default;
        VKWindow(const std::string &title, int width, int height, bool full = false);
        virtual ~VKWindow() { }
        void initWindow(const std::string &title, int width, int height, bool full = false);
        virtual void initVulkan();
        void createVertexBuffer();
        void setPath(const std::string &path);
        void loop();
        virtual void proc();
        virtual void cleanup();
        virtual void event(SDL_Event &e) = 0;
        virtual void draw(); 
        void createGraphicsPipeline(); 
        VkShaderModule createShaderModule(const std::vector<char>& code);
        int w = 0, h = 0;
        mxUtil util;
        void quit();
        void setFullScreen(const bool full);
        void updateTexture(SDL_Surface* newSurface);
        void updateTexture(void* pixels, VkDeviceSize imageSize);
        void printText(const std::string &text, int x, int y, const SDL_Color &col);
        void clearTextQueue();
        
        // Raycaster player data (set by derived class, used by draw())
        struct {
            float posX = 8.0f, posY = 2.0f;
            float dirX = 0.0f, dirY = 1.0f;
            float planeX = 0.66f, planeY = 0.0f;
            float bubbleEffect = 0.0f;
            float kaleidoEffect = 0.0f;
        } raycastPlayer;
        
    protected:
        bool active = true;
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
        VkSampler textureSampler = VK_NULL_HANDLE;
        
        VkImage floorTextureImage = VK_NULL_HANDLE;
        VkDeviceMemory floorTextureImageMemory = VK_NULL_HANDLE;
        VkImageView floorTextureImageView = VK_NULL_HANDLE;
        VkSampler floorTextureSampler = VK_NULL_HANDLE;
        
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
        void createFloorTextureImage(SDL_Surface* surfacex);
        void setupFloorTextureImage(uint32_t w, uint32_t h);
        void createFloorTextureImageView();
        void createFloorTextureSampler();
        void createDescriptorPool();
        void createDescriptorSets();
        void updateDescriptorSets();
        void recreateSwapChain();
        void setupTextureImage(uint32_t w, uint32_t h);
        void createTextDescriptorSetLayout();
        void createTextPipeline();
        void createTextDescriptorPool();
    };

}

#endif
