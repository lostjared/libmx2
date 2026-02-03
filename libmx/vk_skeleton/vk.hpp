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
        alignas(16) glm::vec4 params;      
        alignas(16) glm::vec4 color;  
        alignas(16) glm::vec4 playerPos;   
        alignas(16) glm::vec4 playerPlane; 
    };

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

    class VKSprite {
    public:
        VKSprite(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue,
                 VkCommandPool commandPool);
        ~VKSprite();
        
        void loadSprite(const std::string &pngPath, const std::string &fragmentShaderPath = "");
        void loadSprite(SDL_Surface* surface, const std::string &fragmentShaderPath = "");
        void drawSprite(int x, int y);
        void drawSprite(int x, int y, float scaleX, float scaleY);
        void drawSprite(int x, int y, float scaleX, float scaleY, float rotation);
        void drawSpriteRect(int x, int y, int w, int h);
        void setShaderParams(float p1 = 0.0f, float p2 = 0.0f, float p3 = 0.0f, float p4 = 0.0f);
        void renderSprites(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout,
                          uint32_t screenWidth, uint32_t screenHeight);
        void clearQueue();
        int getWidth() const { return spriteWidth; }
        int getHeight() const { return spriteHeight; }
        void setDescriptorSetLayout(VkDescriptorSetLayout layout) { descriptorSetLayout = layout; }
        VkSampler spriteSampler = VK_NULL_HANDLE;
        
    private:
        struct SpriteVertex {
            float pos[2];
            float texCoord[2];
        };
        
        
        struct SpriteDrawCmd {
            float x, y, w, h;  
        };
        
        VkDevice device = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkImage spriteImage = VK_NULL_HANDLE;
        VkDeviceMemory spriteImageMemory = VK_NULL_HANDLE;
        VkImageView spriteImageView = VK_NULL_HANDLE;
        int spriteWidth = 0;
        int spriteHeight = 0;
        bool spriteLoaded = false;
        VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
        bool hasCustomShader = false;
        glm::vec4 shaderParams = glm::vec4(0.0f);
        std::vector<SpriteDrawCmd> drawQueue;  
        
        
        VkBuffer quadVertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory quadVertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer quadIndexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory quadIndexBufferMemory = VK_NULL_HANDLE;
        bool quadBufferCreated = false;
        void createQuadBuffer();
        
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
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
        void createSampler();
        SDL_Surface* convertToRGBA(SDL_Surface* surface);
        void createSpriteTexture(SDL_Surface* surface);
        VkShaderModule createShaderModule(const std::vector<char>& code);
        std::vector<char> readShaderFile(const std::string& filename);
    };

    class VKWindow {
    public:
        VKWindow() = default;
        VKWindow(const std::string &title, int width, int height, bool full = false);
        virtual ~VKWindow() { }
        void initWindow(const std::string &title, int width, int height, bool full = false);
        void setFont(const std::string &font, const int size = 24);
        virtual void initVulkan();
        void createVertexBuffer();
        void setPath(const std::string &path);
        void loop();
        virtual void proc();
        virtual void cleanup();
        virtual void event(SDL_Event &e) = 0;
        virtual void draw(); 
        virtual bool shouldRender3D() { return true; } 
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
        VKSprite* createSprite(const std::string &pngPath, const std::string &fragmentShaderPath = "");
        VKSprite* createSprite(SDL_Surface* surface, const std::string &fragmentShaderPath = "");
        void renderSprites();
        int getWidth() const { return w; }
        int getHeight() const { return h; }
        struct {
            float posX = 8.0f, posY = 2.0f;
            float dirX = 0.0f, dirY = 1.0f;
            float planeX = 0.66f, planeY = 0.0f;
        } raycastPlayer;
        
    protected:
        bool active = true;
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
