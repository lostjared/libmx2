#ifndef _VK_MODEL_MX_H__
#define _VK_MODEL_MX_H__

#include "config.h"

#ifdef WITH_MOLTEN
#include <vulkan/vulkan.h>
#else
#include "volk.h"
#endif

#include "exception.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <unordered_map>
#include <functional>
#include <iostream>
#include "util.hpp"

namespace mx {

    struct VKVertex {
        float pos[3];
        float texCoord[2];
        float normal[3];

        bool operator==(const VKVertex &other) const {
            return std::memcmp(this, &other, sizeof(VKVertex)) == 0;
        }
    };

    struct VKVertexHash {
        std::size_t operator()(const VKVertex &v) const {
            std::size_t seed = 0;
            auto hc = [&seed](float val) {
                std::hash<float> h;
                seed ^= h(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };
            for (int i = 0; i < 3; ++i) hc(v.pos[i]);
            for (int i = 0; i < 2; ++i) hc(v.texCoord[i]);
            for (int i = 0; i < 3; ++i) hc(v.normal[i]);
            return seed;
        }
    };

    class MXModel {
    public:
        MXModel() = default;
        ~MXModel() = default;
        void load(const std::string &path, float positionScale = 1.0f);
        void upload(VkDevice device, VkPhysicalDevice physicalDevice,
                    VkCommandPool commandPool, VkQueue graphicsQueue);
        void cleanup(VkDevice device);
        void draw(VkCommandBuffer cmd) const;
        void compressIndices();

        const std::vector<VKVertex>&  vertices() const { return vertices_; }
        const std::vector<uint32_t>&  indices()  const { return indices_; }
        uint32_t indexCount() const { return static_cast<uint32_t>(indices_.size()); }

        VkBuffer vertexBuffer() const { return vertexBuffer_; }
        VkBuffer indexBuffer()  const { return indexBuffer_; }
        uint32_t textureIndex() const { return textureIndex_; }
    private:
        std::vector<VKVertex>  vertices_;
        std::vector<uint32_t>  indices_;
        uint32_t textureIndex_ = 0;

        VkBuffer       vertexBuffer_       = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
        VkBuffer       indexBuffer_        = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory_  = VK_NULL_HANDLE;

        static void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                                 VkDeviceSize size, VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags properties,
                                 VkBuffer &buffer, VkDeviceMemory &bufferMemory);
        static uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
                                       uint32_t typeFilter, VkMemoryPropertyFlags properties);
        static void copyBuffer(VkDevice device, VkCommandPool commandPool,
                               VkQueue graphicsQueue,
                               VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    };
}

#endif
