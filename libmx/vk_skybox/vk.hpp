#ifndef _VK_SKYBOX_H__
#define _VK_SKYBOX_H__

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
#include "vk_model.hpp"

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
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define VK_CHECK_RESULT(f) { \
    VkResult res = (f); \
    if (res != VK_SUCCESS) { \
        throw mx::Exception("Fatal : VkResult is \"" + std::to_string(res) + "\" in " + __FILE__ + " at line " + std::to_string(__LINE__)); \
    } \
}

namespace mx {

    using Vertex = VKVertex;

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

    struct SkyboxUBO {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
        alignas(16) glm::vec4 params; 
    };

    class SkyboxViewer {
    public:
        SkyboxViewer() = default;
        SkyboxViewer(const std::string &title, int width, int height, bool full = false);
        virtual ~SkyboxViewer() {}

        void initWindow(const std::string &title, int width, int height, bool full = false);
        void initVulkan();
        void setPath(const std::string &path);
        void loop();
        void cleanup();
        void event(SDL_Event &e);
        void draw();
        void quit();

        int w = 0, h = 0;
        mxUtil util;

    protected:
        bool active = true;

        MXModel model;

        VkInstance instance = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

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
        std::vector<VkFramebuffer> swapChainFramebuffers;

        VkCommandPool commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> commandBuffers;

        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

        
        VkImage cubemapImage = VK_NULL_HANDLE;
        VkDeviceMemory cubemapImageMemory = VK_NULL_HANDLE;
        VkImageView cubemapImageView = VK_NULL_HANDLE;
        VkSampler cubemapSampler = VK_NULL_HANDLE;

        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
        VkImageView depthImageView = VK_NULL_HANDLE;
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets;

        float yaw = 0.0f;
        float pitch = 0.0f;
        bool mouseControl = false;
        int lastMouseX = 0, lastMouseY = 0;
        float mouseSensitivity = 0.15f;

        SDL_Window *window = nullptr;

        void createInstance();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void createDepthResources();
        void createDescriptorSetLayout();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();
        void createUniformBuffers();
        void loadModelData();
        void loadCubemapTexture();
        void createCubemapSampler();
        void createDescriptorPool();
        void createDescriptorSets();
        void cleanupSwapChain();
        void recreateSwapChain();

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        bool isDeviceSuitable(VkPhysicalDevice device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        VkShaderModule createShaderModule(const std::vector<char>& code);

        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount = 1);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t w, uint32_t h, uint32_t layer = 0);
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        VkFormat findDepthFormat();
    };
}

#endif
