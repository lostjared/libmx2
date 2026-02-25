/**
 * @file vk_sprite.hpp
 * @brief Vulkan 2-D sprite renderer with optional custom shaders and instancing.
 *
 * VKSprite manages a Vulkan texture, a screen-space quad, and an optional
 * custom graphics pipeline.  It supports:
 * - Loading images from PNG files or SDL_Surface objects.
 * - Drawing at arbitrary positions, scales, and rotations.
 * - GPU instancing for large batches of identical sprites.
 * - Extended UBO with mouse state and four custom vec4 uniforms.
 */
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

/**
 * @class VKSprite
 * @brief Vulkan 2-D sprite with texture, custom shader, and instancing support.
 *
 * Allocates and manages all Vulkan resources required to render one sprite
 * type: image, sampler, descriptor set, vertex/index buffers, and an
 * optional custom pipeline.  Multiple draw commands are batched into a
 * queue and submitted together in renderSprites().
 */
    class VKSprite {
    public:
        /**
         * @brief Construct and record Vulkan context handles.
         * @param device         Logical device.
         * @param physicalDevice Physical device.
         * @param graphicsQueue  Graphics queue.
         * @param commandPool    Command pool for staging operations.
         */
        VKSprite(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue,
                 VkCommandPool commandPool);

        /** @brief Destructor — frees all Vulkan resources. */
        ~VKSprite();

        VKSprite(const VKSprite&) = delete;
        VKSprite& operator=(const VKSprite&) = delete;
        VKSprite(VKSprite&&) = delete;
        VKSprite& operator=(VKSprite&&) = delete;

        /**
         * @brief Load sprite texture from a PNG file.
         * @param pngPath            Path to the PNG file.
         * @param fragmentShaderPath Optional custom fragment shader (SPIR-V .spv).
         */
        void loadSprite(const std::string &pngPath, const std::string &fragmentShaderPath = "");

        /**
         * @brief Load sprite texture from an SDL_Surface.
         * @param surface            Source surface (not consumed).
         * @param fragmentShaderPath Optional custom fragment shader.
         */
        void loadSprite(SDL_Surface* surface, const std::string &fragmentShaderPath = "");

        /**
         * @brief Create a blank (un-initialised) sprite texture.
         * @param width              Pixel width.
         * @param height             Pixel height.
         * @param vertexShaderPath   Optional vertex shader.
         * @param fragmentShaderPath Optional fragment shader.
         */
        void createEmptySprite(int width, int height, const std::string &vertexShaderPath = "", const std::string &fragmentShaderPath = "");

        /**
         * @brief Queue a draw at the given pixel position.
         * @param x Destination X.
         * @param y Destination Y.
         */
        void drawSprite(int x, int y);

        /**
         * @brief Queue a scaled draw.
         * @param x      Destination X.
         * @param y      Destination Y.
         * @param scaleX Horizontal scale factor.
         * @param scaleY Vertical scale factor.
         */
        void drawSprite(int x, int y, float scaleX, float scaleY);

        /**
         * @brief Queue a scaled and rotated draw.
         * @param x        Destination X.
         * @param y        Destination Y.
         * @param scaleX   Horizontal scale.
         * @param scaleY   Vertical scale.
         * @param rotation Clockwise rotation in degrees.
         */
        void drawSprite(int x, int y, float scaleX, float scaleY, float rotation);

        /**
         * @brief Queue a draw into an explicit destination rectangle.
         * @param x,y,w,h Destination rectangle.
         */
        void drawSpriteRect(int x, int y, int w, int h);

        /**
         * @brief Replace the sprite texture from an SDL_Surface.
         * @param surface New surface (not consumed).
         */
        void updateTexture(SDL_Surface* surface);

        /**
         * @brief Replace the sprite texture from a raw pixel buffer.
         * @param pixels Pointer to RGBA data.
         * @param width  Buffer width.
         * @param height Buffer height.
         * @param pitch  Row stride in bytes (0 = auto).
         */
        void updateTexture(const void* pixels, int width, int height, int pitch = 0);

        /**
         * @brief Set up to four custom shader float parameters.
         * @param p1,p2,p3,p4 Parameter values packed into a vec4.
         */
        void setShaderParams(float p1 = 0.0f, float p2 = 0.0f, float p3 = 0.0f, float p4 = 0.0f);

        /**
         * @brief Enable or disable the custom fragment shader effects.
         * @param enabled @c true to enable effects.
         */
        void setEffectsEnabled(bool enabled) { effectsEnabled = enabled; }

        /** @return @c true if shader effects are enabled. */
        bool getEffectsEnabled() const { return effectsEnabled; }

        /**
         * @brief Record all queued draw commands into the given command buffer.
         * @param cmdBuffer    Active command buffer.
         * @param pipelineLayout Pipeline layout for push constants/descriptors.
         * @param screenWidth  Current viewport width.
         * @param screenHeight Current viewport height.
         */
        void renderSprites(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout,
                          uint32_t screenWidth, uint32_t screenHeight);

        /** @brief Discard all pending draw commands without rendering. */
        void clearQueue();

        /** @return Sprite texture width in pixels. */
        int getWidth() const { return spriteWidth; }
        /** @return Sprite texture height in pixels. */
        int getHeight() const { return spriteHeight; }

        /** @brief Assign an external descriptor-set layout. */
        void setDescriptorSetLayout(VkDescriptorSetLayout layout) { descriptorSetLayout = layout; }
        /** @brief Assign the render pass used to build the custom pipeline. */
        void setRenderPass(VkRenderPass rp) { renderPass = rp; }
        /** @brief Override the vertex shader path (used when rebuilding the pipeline). */
        void setVertexShaderPath(const std::string &path) { vertexShaderPath = path; }

        /** @return @c true if a custom pipeline has been built. */
        bool hasOwnPipeline() const { return customPipeline != VK_NULL_HANDLE; }
        /** @return The custom VkPipeline handle (may be VK_NULL_HANDLE). */
        VkPipeline getPipeline() const { return customPipeline; }
        /** @return The custom pipeline layout handle. */
        VkPipelineLayout getPipelineLayout() const { return customPipelineLayout; }

        /** @brief Destroy and recreate the custom graphics pipeline. */
        void rebuildPipeline();
        /** @brief Destroy and recreate the instanced graphics pipeline. */
        void rebuildInstancedPipeline();

        VkSampler spriteSampler = VK_NULL_HANDLE; ///< Texture sampler.

        /**
         * @brief Enable GPU instancing for this sprite type.
         * @param maxInstances          Maximum simultaneous instances.
         * @param instanceVertShaderPath Vertex shader supporting instancing.
         * @param instanceFragShaderPath Fragment shader.
         */
        void enableInstancing(uint32_t maxInstances,
                              const std::string &instanceVertShaderPath,
                              const std::string &instanceFragShaderPath);

        /** @return @c true if GPU instancing is active. */
        bool isInstancingEnabled() const { return instancingEnabled; }

        /** @brief Allocate and initialise the extended uniform buffer object. */
        void enableExtendedUBO();

        /** @return @c true if the extended UBO is active. */
        bool isExtendedUBOEnabled() const { return extendedUBOEnabled; }

        /**
         * @brief Upload mouse state to the extended UBO.
         * @param mx       Mouse X (normalised or pixels).
         * @param my       Mouse Y.
         * @param pressed  Mouse button state.
         * @param reserved Reserved channel.
         */
        void setMouseState(float mx, float my, float pressed, float reserved = 0.0f);

        /** @brief Upload user uniform 0 to the extended UBO. @param x,y,z,w Components. */
        void setUniform0(float x, float y, float z, float w);
        /** @brief Upload user uniform 1 to the extended UBO. @param x,y,z,w Components. */
        void setUniform1(float x, float y, float z, float w);
        /** @brief Upload user uniform 2 to the extended UBO. @param x,y,z,w Components. */
        void setUniform2(float x, float y, float z, float w);
        /** @brief Upload user uniform 3 to the extended UBO. @param x,y,z,w Components. */
        void setUniform3(float x, float y, float z, float w);
        
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
    
        struct SpriteExtendedUBO {
            glm::vec4 mouse;   
            glm::vec4 u0;      
            glm::vec4 u1;
            glm::vec4 u2;
            glm::vec4 u3;
        };
        bool extendedUBOEnabled = false;
        SpriteExtendedUBO extendedUBOData{};
        VkBuffer extendedUBOBuffer = VK_NULL_HANDLE;
        VkDeviceMemory extendedUBOMemory = VK_NULL_HANDLE;
        void* extendedUBOMapped = nullptr;
        VkDescriptorSetLayout extendedDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool extendedDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet extendedDescriptorSet = VK_NULL_HANDLE;
        bool ownExtendedDescriptorSetLayout = false;
        void createExtendedUBO();
        void updateExtendedUBO();
        void createExtendedDescriptorSetLayout();
        void createExtendedDescriptorSet();
        void destroyExtendedUBO();

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
