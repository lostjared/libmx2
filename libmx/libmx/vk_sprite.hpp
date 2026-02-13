#ifndef __MXSPRITE__
#define __MXSPRITE__

#include"config.h"

#ifdef WITH_MOLTEN
#include<vulkan/vulkan.h>
#else
#include"volk.h"
#endif

#include"exception.hpp"
#include <SDL_vulkan.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>
#include <array>
#include <cstring>
#include <glm/glm.hpp>

#ifndef VK_CHECK_RESULT
#define VK_CHECK_RESULT(f) { \
    VkResult res = (f); \
    if (res != VK_SUCCESS) { \
        throw mx::Exception("Fatal : VkResult is \"" + std::to_string(res) + "\" in " + __FILE__ + " at line " + std::to_string(__LINE__)); \
    } \
}
#endif

namespace mx {

    class VKSprite {
    public:
        VKSprite(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue,
                 VkCommandPool commandPool);
        ~VKSprite();

        VKSprite(const VKSprite&) = delete;
        VKSprite& operator=(const VKSprite&) = delete;
        VKSprite(VKSprite&&) = delete;
        VKSprite& operator=(VKSprite&&) = delete;
        
        void loadSprite(const std::string &pngPath, const std::string &fragmentShaderPath = "");
        void loadSprite(SDL_Surface* surface, const std::string &fragmentShaderPath = "");
        void createEmptySprite(int width, int height, const std::string &vertexShaderPath = "", const std::string &fragmentShaderPath = "");
        void drawSprite(int x, int y);
        void drawSprite(int x, int y, float scaleX, float scaleY);
        void drawSprite(int x, int y, float scaleX, float scaleY, float rotation);
        void drawSpriteRect(int x, int y, int w, int h);
        void updateTexture(SDL_Surface* surface);
        void updateTexture(const void* pixels, int width, int height, int pitch = 0);
        void setShaderParams(float p1 = 0.0f, float p2 = 0.0f, float p3 = 0.0f, float p4 = 0.0f);
        void setEffectsEnabled(bool enabled) { effectsEnabled = enabled; }
        bool getEffectsEnabled() const { return effectsEnabled; }
        void renderSprites(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout,
                          uint32_t screenWidth, uint32_t screenHeight);
        void clearQueue();
        int getWidth() const { return spriteWidth; }
        int getHeight() const { return spriteHeight; }
        void setDescriptorSetLayout(VkDescriptorSetLayout layout) { descriptorSetLayout = layout; }
        void setRenderPass(VkRenderPass rp) { renderPass = rp; }
        void setVertexShaderPath(const std::string &path) { vertexShaderPath = path; }
        bool hasOwnPipeline() const { return customPipeline != VK_NULL_HANDLE; }
        VkPipeline getPipeline() const { return customPipeline; }
        VkPipelineLayout getPipelineLayout() const { return customPipelineLayout; }
        void rebuildPipeline();  
        void rebuildInstancedPipeline();  
        VkSampler spriteSampler = VK_NULL_HANDLE;
        void enableInstancing(uint32_t maxInstances,
                              const std::string &instanceVertShaderPath,
                              const std::string &instanceFragShaderPath);
        bool isInstancingEnabled() const { return instancingEnabled; }
        
    private:
        struct SpriteVertex {
            float pos[2];
            float texCoord[2];
        };
        
        struct SpriteDrawCmd {
            float x, y, w, h;
            glm::vec4 params;  
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
        bool effectsEnabled = true;
        std::vector<SpriteDrawCmd> drawQueue;  
        
        VkPipeline customPipeline = VK_NULL_HANDLE;
        VkPipelineLayout customPipelineLayout = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        std::string vertexShaderPath;
        void createCustomPipeline();
        
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
        void destroySpriteResources();
        
        VkBuffer persistentStagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory persistentStagingMemory = VK_NULL_HANDLE;
        void* persistentStagingMapped = nullptr;
        VkDeviceSize persistentStagingSize = 0;
        VkFence uploadFence = VK_NULL_HANDLE;
        VkCommandBuffer uploadCmdBuffer = VK_NULL_HANDLE;
        bool stagingResourcesCreated = false;
        void createStagingResources(VkDeviceSize size);
        void destroyStagingResources();
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
        void updateSpriteTexture(const void* pixels, uint32_t width, uint32_t height);
        VkShaderModule createShaderModule(const std::vector<char>& code);
        std::vector<char> readShaderFile(const std::string& filename);
    
        struct SpriteInstanceData {
            float posX, posY, sizeW, sizeH;
            float params[4];
        };
        VkBuffer instanceBuffer = VK_NULL_HANDLE;
        VkDeviceMemory instanceBufferMemory = VK_NULL_HANDLE;
        void* instanceBufferMapped = nullptr;
        uint32_t instanceBufferCapacity = 0;
        bool instancingEnabled = false;
        VkPipeline instancedPipeline = VK_NULL_HANDLE;
        VkPipelineLayout instancedPipelineLayout = VK_NULL_HANDLE;
        std::string instanceVertPath_;
        std::string instanceFragPath_;
        void createInstancedPipeline(const std::string &vertPath, const std::string &fragPath);
        void ensureInstanceBuffer(uint32_t count);
        void destroyInstanceResources();
    };

}

#endif
