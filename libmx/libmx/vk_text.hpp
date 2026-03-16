/**
 * @file vk_text.hpp
 * @brief Vulkan SDL_ttf text renderer.
 *
 * VKText rasterises text strings into SDL surfaces, uploads them as
 * Vulkan image textures, and batches the resulting quads for submission
 * during the render pass.  Each printTextG_Solid() call adds one TextQuad
 * to the pending queue, which is flushed by renderText().
 */
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
#include <unordered_map>

#ifndef VK_CHECK_RESULT
#define VK_CHECK_RESULT(f) { \
    VkResult res = (f); \
    if (res != VK_SUCCESS) { \
        throw mx::Exception("Fatal : VkResult is \"" + std::to_string(res) + "\" in " + __FILE__ + " at line " + std::to_string(__LINE__)); \
    } \
}
#endif

namespace mx {

/**
 * @class VKText
 * @brief Renders SDL_ttf text into Vulkan image textures.
 *
 * Loads a TrueType font via SDL_ttf, creates a descriptor pool for per-glyph
 * textures, and provides a print-then-render workflow:
 * 1. Call printTextG_Solid() for each string to display.
 * 2. Call renderText() once during the render pass to draw all queued strings.
 * 3. Optionally call clearQueue() to discard pending strings.
 */
      /// Cache key combining text content and colour.
      struct CacheKey {
          std::string text;
          uint8_t r, g, b, a;
          bool operator==(const CacheKey &other) const {
              return text == other.text && r == other.r && g == other.g && b == other.b && a == other.a;
          }
      };

      /// Hash functor for CacheKey.
      struct CacheKeyHash {
          size_t operator()(const CacheKey &k) const {
              size_t h = std::hash<std::string>{}(k.text);
              h ^= std::hash<uint8_t>{}(k.r) + 0x9e3779b9 + (h << 6) + (h >> 2);
              h ^= std::hash<uint8_t>{}(k.g) + 0x9e3779b9 + (h << 6) + (h >> 2);
              h ^= std::hash<uint8_t>{}(k.b) + 0x9e3779b9 + (h << 6) + (h >> 2);
              h ^= std::hash<uint8_t>{}(k.a) + 0x9e3779b9 + (h << 6) + (h >> 2);
              return h;
          }
      };

      /// GPU texture resources cached for a rendered text string.
      struct CachedTexture {
          VkImage image = VK_NULL_HANDLE;
          VkDeviceMemory imageMemory = VK_NULL_HANDLE;
          VkImageView imageView = VK_NULL_HANDLE;
          int width = 0;
          int height = 0;
      };

      class VKText {
        public:
        /**
         * @brief Construct VKText and load the font.
         * @param device         Logical device.
         * @param physicalDevice Physical device.
         * @param graphicsQueue  Graphics queue.
         * @param commandPool    Command pool for staging.
         * @param fontPath       Path to the TTF font file.
         * @param fontSize       Point size.
         */
        VKText(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, 
               VkCommandPool commandPool, const std::string &fontPath, int fontSize = 24);

        /** @brief Destructor -- destroys all Vulkan and SDL_ttf resources. */
        ~VKText();

        VKText(const VKText&) = delete;
        VKText& operator=(const VKText&) = delete;
        VKText(VKText&&) = delete;
        VKText& operator=(VKText&&) = delete;

        /**
         * @brief Queue a text string for solid (opaque) rendering.
         * @param text String to draw.
         * @param x    Destination X in pixels.
         * @param y    Destination Y in pixels.
         * @param col  Text colour.
         */
        void printTextG_Solid(const std::string &text, int x, int y, const SDL_Color &col);

        /**
         * @brief Record all queued text quads into a command buffer.
         * @param cmdBuffer    Active Vulkan command buffer.
         * @param pipelineLayout Pipeline layout for push constants.
         * @param screenWidth  Viewport width.
         * @param screenHeight Viewport height.
         */
        void renderText(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, 
                       uint32_t screenWidth, uint32_t screenHeight);

        /** @brief Discard all pending text quads without rendering them. */
        void clearQueue();

        /** @brief Assign an external descriptor-set layout. */
        void setDescriptorSetLayout(VkDescriptorSetLayout layout) { descriptorSetLayout = layout; }

        /**
         * @brief Change the active font.
         * @param fontPath New font file path.
         * @param fontSize New point size.
         */
        void setFont(const std::string &fontPath, int fontSize);

        /** @brief Destroy all cached text textures, freeing GPU memory. */
        void clearCache();

        /**
         * @brief Measure the pixel dimensions of a string with the current font.
         * @param text   String to measure.
         * @param width  Output: pixel width.
         * @param height Output: pixel height.
         * @return @c true if measurement succeeded.
         */
        bool getTextDimensions(const std::string &text, int& width, int& height);

        VkSampler fontSampler = VK_NULL_HANDLE; ///< Sampler used for all text textures.
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
            bool ownsTexture = true; ///< false when texture is owned by the cache.

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
                    ownsTexture = other.ownsTexture;

                    other.ownsTexture = false;
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
                    if (ownsTexture) {
                        if (textImageView != VK_NULL_HANDLE) {
                            vkDestroyImageView(device, textImageView, nullptr);
                        }
                        if (textImage != VK_NULL_HANDLE) {
                            vkDestroyImage(device, textImage, nullptr);
                            vkFreeMemory(device, textImageMemory, nullptr);
                        }
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
        uint32_t maxPoolSets = 100;
        std::unordered_map<CacheKey, CachedTexture, CacheKeyHash> textureCache;
        
        void initFont(const std::string &fontPath, int fontSize);
        void createDescriptorPool();
        void createDescriptorPool(uint32_t maxSets);
        void growDescriptorPool();
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