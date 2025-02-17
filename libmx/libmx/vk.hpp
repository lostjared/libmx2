#ifndef _VK_MX_H__
#define _VK_MX_H__
#include "mx.hpp"
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <fstream>
#include <algorithm>
#include <array>

namespace mx {

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

    class VKWindow {
    public:
        VKWindow() = default;
        VKWindow(const std::string &title, int width, int height);
        virtual ~VKWindow() { cleanup() ; }
        void initWindow(const std::string &title, int width, int height);
        virtual void initVulkan();
        void loop();
        void proc();
        void cleanup();
        virtual void event(SDL_Event &e) = 0;
        virtual void draw(); // overload this function to draw
        virtual void createGraphicsPipeline() = 0; // overload to setup shaders/custom drawing
        VkShaderModule createShaderModule(const std::vector<char>& code);
        int w = 0, h = 0;
    protected:
        VkInstance instance = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;


        VkSwapchainKHR swapChain = VK_NULL_HANDLE;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImageViews;


        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> swapChainFramebuffers;


        VkCommandPool commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> commandBuffers;


        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

        SDL_Window *window = nullptr;

        void createInstance();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
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
    
    };

}

#endif