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
#include "util.hpp"
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace mx {

    /**
     * @struct VKVertex
     * @brief Per-vertex data uploaded to the Vulkan vertex buffer.
     */
    struct VKVertex {
        float pos[3];      ///< 3-D position (x, y, z).
        float texCoord[2]; ///< UV texture coordinates.
        float normal[3];   ///< Surface normal vector.

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
            for (int i = 0; i < 3; ++i)
                hc(v.pos[i]);
            for (int i = 0; i < 2; ++i)
                hc(v.texCoord[i]);
            for (int i = 0; i < 3; ++i)
                hc(v.normal[i]);
            return seed;
        }
    };

    /* Load textures per sub-mesh
   for (size_t i = 0; i < model.subMeshCount(); ++i) {
       uint32_t texIdx = model.subMesh(i).textureIndex;
       // bind texture[texIdx] to descriptor set
       model.drawSubMesh(cmd, i);
   }*/

    /**
     * @struct SubMesh
     * @brief A range of indices within the shared index buffer, with its own texture.
     */
    struct SubMesh {
        uint32_t firstIndex = 0;   ///< Offset into the index buffer.
        uint32_t indexCount = 0;   ///< Number of indices in this sub-mesh.
        uint32_t textureIndex = 0; ///< Texture slot for this sub-mesh.
        std::string materialName;  ///< Material name from OBJ usemtl directive.
    };

    /**
     * @struct MXMaterial
     * @brief Parsed Wavefront MTL material properties.
     */
    struct MXMaterial {
        std::string name;                 ///< Material name (newmtl).
        float ka[3] = {0.2f, 0.2f, 0.2f}; ///< Ambient colour.
        float kd[3] = {0.8f, 0.8f, 0.8f}; ///< Diffuse colour.
        float ks[3] = {0.0f, 0.0f, 0.0f}; ///< Specular colour.
        float ns = 30.0f;                 ///< Specular exponent.
        float d = 1.0f;                   ///< Opacity (dissolve).
        int illum = 2;                    ///< Illumination model.
        std::string map_kd;               ///< Diffuse texture filename.
    };

    /**
     * @class MXModel
     * @brief Vulkan mesh loader and GPU resource manager.
     *
     * Loads a Wavefront OBJ file, optionally deduplicates vertices
     * (compressIndices), uploads the geometry to device-local Vulkan buffers,
     * and provides a draw() helper that records indexed draw commands.
     * Supports multiple sub-meshes (groups) with separate texture indices.
     */
    class MXModel {
      public:
        /** @brief Default constructor. */
        MXModel() = default;
        /** @brief Destructor. */
        ~MXModel() = default;

        MXModel(const MXModel &) = delete;
        MXModel &operator=(const MXModel &) = delete;
        MXModel(MXModel &&) = delete;
        MXModel &operator=(MXModel &&) = delete;

        /**
         * @brief Parse a model file (.mxmod, .mxmod.z, or .obj) and populate the vertex/index vectors.
         * @param path          Path to the model file.
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
        const std::vector<VKVertex> &vertices() const { return vertices_; }
        /** @return Read-only reference to the index array. */
        const std::vector<uint32_t> &indices() const { return indices_; }
        /** @return Number of indices in the index buffer. */
        uint32_t indexCount() const { return static_cast<uint32_t>(indices_.size()); }

        /** @return Vulkan vertex buffer handle. */
        VkBuffer vertexBuffer() const { return vertexBuffer_; }
        /** @return Vulkan index buffer handle. */
        VkBuffer indexBuffer() const { return indexBuffer_; }
        /** @return Texture sub-mesh index encoded in the OBJ file (first sub-mesh). */
        uint32_t textureIndex() const { return textureIndex_; }

        /** @return Number of sub-meshes (groups) in the model. */
        size_t subMeshCount() const { return subMeshes_.size(); }
        /** @return Read-only reference to a sub-mesh by index. */
        const SubMesh &subMesh(size_t i) const { return subMeshes_[i]; }
        /** @return Read-only reference to all sub-meshes. */
        const std::vector<SubMesh> &subMeshes() const { return subMeshes_; }

        /**
         * @brief Draw a single sub-mesh (binds vertex/index buffers and issues indexed draw).
         * @param cmd   Active command buffer.
         * @param index Sub-mesh index.
         */
        void drawSubMesh(VkCommandBuffer cmd, size_t index) const;

        /** @return Read-only reference to parsed MTL materials. */
        const std::vector<MXMaterial> &materials() const { return materials_; }
        /** @return Path to the MTL library referenced by the OBJ, or empty. */
        const std::string &mtlLibPath() const { return mtlLibPath_; }

      private:
        std::vector<VKVertex> vertices_;    ///< Parsed vertex data.
        std::vector<uint32_t> indices_;     ///< Parsed index data.
        uint32_t textureIndex_ = 0;         ///< Sub-mesh texture index.
        std::vector<SubMesh> subMeshes_;    ///< Sub-mesh ranges (one per group).
        std::vector<MXMaterial> materials_; ///< Parsed MTL materials.
        std::string mtlLibPath_;            ///< Path to .mtl file from OBJ mtllib.

        VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
        VkBuffer indexBuffer_ = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory_ = VK_NULL_HANDLE;

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

        /** @brief Load a Wavefront OBJ file. */
        void loadOBJ(const std::string &path, float positionScale);

        /** @brief Load an MXMOD format file. */
        void loadMXMOD(const std::string &path, float positionScale);

        /** @brief Parse a Wavefront .mtl file and populate materials_. */
        void loadMTL(const std::string &path);
    };
} // namespace mx

#endif
