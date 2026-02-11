#ifndef __MXTEXT__
#define __MXTEXT__

#include"config.h"

#ifdef WITH_MOLTEN
#include<vulkan/vulkan.h>
#else
#include"volk.h"
#endif

#include"exception.hpp"
#include <SDL_vulkan.h>
#include <SDL_ttf.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <string>
#include <cstring>

#ifndef VK_CHECK_RESULT
#define VK_CHECK_RESULT(f) { \
    VkResult res = (f); \
    if (res != VK_SUCCESS) { \
        throw mx::Exception("Fatal : VkResult is \"" + std::to_string(res) + "\" in " + __FILE__ + " at line " + std::to_string(__LINE__)); \
    } \
}
#endif

namespace mx {
      class VKText {
        public:
        VKText(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, 
               VkCommandPool commandPool, const std::string &fontPath, int fontSize = 24);
        ~VKText();
        void printTextG_Solid(const std::string &text, int x, int y, const SDL_Color &col);
        void renderText(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, 
                       uint32_t screenWidth, uint32_t screenHeight);
        void clearQueue();
        void setDescriptorSetLayout(VkDescriptorSetLayout layout) { descriptorSetLayout = layout; }
        void setFont(const std::string &fontPath, int fontSize);
        bool getTextDimensions(const std::string &text, int& width, int& height);
        VkSampler fontSampler = VK_NULL_HANDLE;
        private:
        struct TextVertex {
            float pos[2];
            float texCoord[2];
        };
        
        struct TextQuad {
            std::string text;
            int x, y;
            int width, height;
            SDL_Color color;
            std::vector<TextVertex> vertices;
            std::vector<uint16_t> indices;
            VkBuffer vertexBuffer = VK_NULL_HANDLE;
            VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
            VkBuffer indexBuffer = VK_NULL_HANDLE;
            VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
            VkImage textImage = VK_NULL_HANDLE;
            VkDeviceMemory textImageMemory = VK_NULL_HANDLE;
            VkImageView textImageView = VK_NULL_HANDLE;
            VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
            uint32_t indexCount = 0;
            VkDevice device = VK_NULL_HANDLE;

            TextQuad() = default;
            TextQuad(const TextQuad&) = delete;
            TextQuad& operator=(const TextQuad&) = delete;
            TextQuad(TextQuad&& other) noexcept {
                *this = std::move(other);
            }
            TextQuad& operator=(TextQuad&& other) noexcept {
                if (this != &other) {
                    text = std::move(other.text);
                    x = other.x;
                    y = other.y;
                    width = other.width;
                    height = other.height;
                    color = other.color;
                    vertices = std::move(other.vertices);
                    indices = std::move(other.indices);
                    vertexBuffer = other.vertexBuffer;
                    vertexBufferMemory = other.vertexBufferMemory;
                    indexBuffer = other.indexBuffer;
                    indexBufferMemory = other.indexBufferMemory;
                    textImage = other.textImage;
                    textImageMemory = other.textImageMemory;
                    textImageView = other.textImageView;
                    descriptorSet = other.descriptorSet;
                    indexCount = other.indexCount;
                    device = other.device;

                    other.vertexBuffer = VK_NULL_HANDLE;
                    other.vertexBufferMemory = VK_NULL_HANDLE;
                    other.indexBuffer = VK_NULL_HANDLE;
                    other.indexBufferMemory = VK_NULL_HANDLE;
                    other.textImage = VK_NULL_HANDLE;
                    other.textImageMemory = VK_NULL_HANDLE;
                    other.textImageView = VK_NULL_HANDLE;
                    other.descriptorSet = VK_NULL_HANDLE;
                    other.indexCount = 0;
                    other.device = VK_NULL_HANDLE;
                }
                return *this;
            }
            
            ~TextQuad() {
                if (device != VK_NULL_HANDLE) {
                    if (textImageView != VK_NULL_HANDLE) {
                        vkDestroyImageView(device, textImageView, nullptr);
                    }
                    if (textImage != VK_NULL_HANDLE) {
                        vkDestroyImage(device, textImage, nullptr);
                        vkFreeMemory(device, textImageMemory, nullptr);
                    }
                    if (vertexBuffer != VK_NULL_HANDLE) {
                        vkDestroyBuffer(device, vertexBuffer, nullptr);
                        vkFreeMemory(device, vertexBufferMemory, nullptr);
                    }
                    if (indexBuffer != VK_NULL_HANDLE) {
                        vkDestroyBuffer(device, indexBuffer, nullptr);
                        vkFreeMemory(device, indexBufferMemory, nullptr);
                    }
                }
            }
        };
        
        VkDevice device = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkCommandPool commandPool = VK_NULL_HANDLE;
        
        TTF_Font *font = nullptr;
        std::vector<TextQuad> textQuads;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        
        void initFont(const std::string &fontPath, int fontSize);
        void createDescriptorPool();
        VkDescriptorSet createDescriptorSet(VkImageView imageView);
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                         VkMemoryPropertyFlags properties, VkBuffer& buffer, 
                         VkDeviceMemory& bufferMemory);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, 
                        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
                        VkImage& image, VkDeviceMemory& imageMemory);
        VkImageView createImageView(VkImage image, VkFormat format);
        SDL_Surface* convertToRGBA(SDL_Surface* surface);
    };
}


#endif