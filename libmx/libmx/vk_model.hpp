/**
 * @file vk_model.hpp
 * @brief Vulkan 3-D model loader and GPU buffer manager.
 *
 * Parses a Wavefront OBJ-format mesh, uploads vertex and index data to
 * device-local Vulkan buffers, and provides a draw call that records
 * bind/draw commands into a command buffer.
 */
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

/**
 * @struct VKVertex
 * @brief Per-vertex data uploaded to the Vulkan vertex buffer.
 */
    struct VKVertex {
        float pos[3];       ///< 3-D position (x, y, z).
        float texCoord[2];  ///< UV texture coordinates.
        float normal[3];    ///< Surface normal vector.

        /**
         * @brief Byte-wise equality comparison for deduplication.
         * @param other Vertex to compare against.
         * @return @c true if every field is identical.
         */
        bool operator==(const VKVertex &other) const {
            return std::memcmp(this, &other, sizeof(VKVertex)) == 0;
        }
    };

/**
 * @struct VKVertexHash
 * @brief Hash functor for VKVertex, suitable for use in unordered_map.
 *
 * Combines std::hash<float> results using a Boost-style seed mix to
 * produce a stable hash of all vertex fields.
 */
    struct VKVertexHash {
        /**
         * @brief Compute the hash of a VKVertex.
         * @param v Vertex to hash.
         * @return Hash value.
         */
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

/**
 * @class MXModel
 * @brief Vulkan mesh loader and GPU resource manager.
 *
 * Loads a Wavefront OBJ file, optionally deduplicates vertices
 * (compressIndices), uploads the geometry to device-local Vulkan buffers,
 * and provides a draw() helper that records indexed draw commands.
 */
    class MXModel {
    public:
        /** @brief Default constructor. */
        MXModel() = default;
        /** @brief Destructor. */
        ~MXModel() = default;

        MXModel(const MXModel&) = delete;
        MXModel& operator=(const MXModel&) = delete;
        MXModel(MXModel&&) = delete;
        MXModel& operator=(MXModel&&) = delete;

        /**
         * @brief Parse an OBJ file and populate the vertex/index vectors.
         * @param path          Path to the .obj file.
         * @param positionScale Uniform scale applied to all vertex positions.
         */
        void load(const std::string &path, float positionScale = 1.0f);

        /**
         * @brief Upload parsed geometry to device-local Vulkan buffers.
         * @param device          Logical device.
         * @param physicalDevice  Physical device (for memory type queries).
         * @param commandPool     Command pool for staging commands.
         * @param graphicsQueue   Queue used to submit staging commands.
         */
        void upload(VkDevice device, VkPhysicalDevice physicalDevice,
                    VkCommandPool commandPool, VkQueue graphicsQueue);

        /**
         * @brief Free all Vulkan buffer resources.
         * @param device Logical device that owns the buffers.
         */
        void cleanup(VkDevice device);

        /**
         * @brief Record a bind + indexed draw into a command buffer.
         * @param cmd Active Vulkan command buffer (must be in recording state).
         */
        void draw(VkCommandBuffer cmd) const;

        /**
         * @brief Deduplicate vertices and rebuild the index list.
         *
         * Reduces GPU bandwidth by removing duplicate VKVertex entries and
         * rewriting the index buffer to reference the canonical copies.
         */
        void compressIndices();

        /** @return Read-only reference to the vertex array. */
        const std::vector<VKVertex>&  vertices() const { return vertices_; }
        /** @return Read-only reference to the index array. */
        const std::vector<uint32_t>&  indices()  const { return indices_; }
        /** @return Number of indices in the index buffer. */
        uint32_t indexCount() const { return static_cast<uint32_t>(indices_.size()); }

        /** @return Vulkan vertex buffer handle. */
        VkBuffer vertexBuffer() const { return vertexBuffer_; }
        /** @return Vulkan index buffer handle. */
        VkBuffer indexBuffer()  const { return indexBuffer_; }
        /** @return Texture sub-mesh index encoded in the OBJ file. */
        uint32_t textureIndex() const { return textureIndex_; }
    private:
        std::vector<VKVertex>  vertices_;  ///< Parsed vertex data.
        std::vector<uint32_t>  indices_;   ///< Parsed index data.
        uint32_t textureIndex_ = 0;        ///< Sub-mesh texture index.

        VkBuffer       vertexBuffer_       = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
        VkBuffer       indexBuffer_        = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory_  = VK_NULL_HANDLE;

        /** @brief Allocate a Vulkan buffer with the specified usage and memory flags. */
        static void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                                 VkDeviceSize size, VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags properties,
                                 VkBuffer &buffer, VkDeviceMemory &bufferMemory);

        /** @brief Find a memory type index satisfying typeFilter and properties. */
        static uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
                                       uint32_t typeFilter, VkMemoryPropertyFlags properties);

        /** @brief Copy bytes from one Vulkan buffer to another via a staging command. */
        static void copyBuffer(VkDevice device, VkCommandPool commandPool,
                               VkQueue graphicsQueue,
                               VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    };
}

#endif
