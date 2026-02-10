#include "vk_model.hpp"

namespace mx {

    struct Vec2 { float x, y; };
    struct Vec3 { float x, y, z; };

    void MXModel::load(const std::string &path, float positionScale) {
        std::ifstream f(path);
        if (!f.is_open())
            throw mx::Exception("MXModel::load: failed to open file: " + path);

        auto readHeader = [&](const char *expectedTag, int &countOut) {
            std::string tag;
            if (!(f >> tag >> countOut))
                throw mx::Exception(std::string("MXModel::load: missing header: ") + expectedTag);
            if (tag != expectedTag)
                throw mx::Exception(std::string("MXModel::load: expected header '") + expectedTag + "', got '" + tag + "'");
        }; 
        {
            std::string triTag;
            int a = 0, b = 0;
            if (!(f >> triTag >> a >> b))
                throw mx::Exception("MXModel::load: missing tri header");
            if (triTag != "tri")
                throw mx::Exception("MXModel::load: expected 'tri' header");
        }
        int vcount = 0;
        readHeader("vert", vcount);
        if (vcount <= 0)
            throw mx::Exception("MXModel::load: invalid vert count");

        std::vector<Vec3> pos(vcount);
        for (int i = 0; i < vcount; ++i) {
            if (!(f >> pos[i].x >> pos[i].y >> pos[i].z))
                throw mx::Exception("MXModel::load: failed reading vertex positions");
            pos[i].x *= positionScale;
            pos[i].y *= positionScale;
            pos[i].z *= positionScale;
        }

        int tcount = 0;
        readHeader("tex", tcount);
        if (tcount != vcount)
            throw mx::Exception("MXModel::load: tex count != vert count");

        std::vector<Vec2> uv(vcount);
        for (int i = 0; i < vcount; ++i) {
            if (!(f >> uv[i].x >> uv[i].y))
                throw mx::Exception("MXModel::load: failed reading texcoords");
        }

        int ncount = 0;
        readHeader("norm", ncount);
        if (ncount != vcount)
            throw mx::Exception("MXModel::load: norm count != vert count");

        std::vector<Vec3> nrm(vcount);
        for (int i = 0; i < vcount; ++i) {
            if (!(f >> nrm[i].x >> nrm[i].y >> nrm[i].z))
                throw mx::Exception("MXModel::load: failed reading normals");
        }
        vertices_.clear();
        vertices_.reserve(static_cast<size_t>(vcount));

        for (int i = 0; i < vcount; ++i) {
            VKVertex v{};
            v.pos[0]      = pos[i].x;
            v.pos[1]      = pos[i].y;
            v.pos[2]      = pos[i].z;
            v.texCoord[0] = uv[i].x;
            v.texCoord[1] = uv[i].y;
            v.normal[0]   = nrm[i].x;
            v.normal[1]   = nrm[i].y;
            v.normal[2]   = nrm[i].z;
            vertices_.push_back(v);
        }

        indices_.clear();
        indices_.reserve(static_cast<size_t>(vcount));
        for (uint32_t i = 0; i < static_cast<uint32_t>(vcount); ++i)
            indices_.push_back(i);
    }

    void MXModel::upload(VkDevice device, VkPhysicalDevice physicalDevice,
                         VkCommandPool commandPool, VkQueue graphicsQueue) {
        
        cleanup(device);

        VkDeviceSize vertexBufferSize = sizeof(vertices_[0]) * vertices_.size();
        VkDeviceSize indexBufferSize  = sizeof(indices_[0])  * indices_.size();

        
        VkBuffer stagingVertexBuffer;
        VkDeviceMemory stagingVertexBufferMemory;
        createBuffer(device, physicalDevice, vertexBufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingVertexBuffer, stagingVertexBufferMemory);

        
        VkBuffer stagingIndexBuffer;
        VkDeviceMemory stagingIndexBufferMemory;
        createBuffer(device, physicalDevice, indexBufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingIndexBuffer, stagingIndexBufferMemory);

        
        void *vertexData = nullptr;
        vkMapMemory(device, stagingVertexBufferMemory, 0, vertexBufferSize, 0, &vertexData);
        memcpy(vertexData, vertices_.data(), static_cast<size_t>(vertexBufferSize));
        vkUnmapMemory(device, stagingVertexBufferMemory);

        
        void *indexData = nullptr;
        vkMapMemory(device, stagingIndexBufferMemory, 0, indexBufferSize, 0, &indexData);
        memcpy(indexData, indices_.data(), static_cast<size_t>(indexBufferSize));
        vkUnmapMemory(device, stagingIndexBufferMemory);

        
        createBuffer(device, physicalDevice, vertexBufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     vertexBuffer_, vertexBufferMemory_);

        
        createBuffer(device, physicalDevice, indexBufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     indexBuffer_, indexBufferMemory_);

        
        copyBuffer(device, commandPool, graphicsQueue, stagingVertexBuffer, vertexBuffer_, vertexBufferSize);
        copyBuffer(device, commandPool, graphicsQueue, stagingIndexBuffer,  indexBuffer_,  indexBufferSize);

        
        vkDestroyBuffer(device, stagingVertexBuffer, nullptr);
        vkFreeMemory(device, stagingVertexBufferMemory, nullptr);
        vkDestroyBuffer(device, stagingIndexBuffer, nullptr);
        vkFreeMemory(device, stagingIndexBufferMemory, nullptr);
    }

    void MXModel::cleanup(VkDevice device) {
        if (vertexBuffer_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, vertexBuffer_, nullptr);
            vkFreeMemory(device, vertexBufferMemory_, nullptr);
            vertexBuffer_       = VK_NULL_HANDLE;
            vertexBufferMemory_ = VK_NULL_HANDLE;
        }
        if (indexBuffer_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, indexBuffer_, nullptr);
            vkFreeMemory(device, indexBufferMemory_, nullptr);
            indexBuffer_       = VK_NULL_HANDLE;
            indexBufferMemory_ = VK_NULL_HANDLE;
        }
    }

    void MXModel::draw(VkCommandBuffer cmd) const {
        if (vertexBuffer_ == VK_NULL_HANDLE || indexBuffer_ == VK_NULL_HANDLE)
            return;

        VkBuffer buffers[] = { vertexBuffer_ };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
        vkCmdBindIndexBuffer(cmd, indexBuffer_, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, indexCount(), 1, 0, 0, 0);
    }

    

    void MXModel::createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                               VkDeviceSize size, VkBufferUsageFlags usage,
                               VkMemoryPropertyFlags properties,
                               VkBuffer &buffer, VkDeviceMemory &bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size        = size;
        bufferInfo.usage       = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            throw mx::Exception("MXModel: Failed to create buffer!");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
            throw mx::Exception("MXModel: Failed to allocate buffer memory!");

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    uint32_t MXModel::findMemoryType(VkPhysicalDevice physicalDevice,
                                     uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw mx::Exception("MXModel: Failed to find suitable memory type!");
    }

    void MXModel::copyBuffer(VkDevice device, VkCommandPool commandPool,
                             VkQueue graphicsQueue,
                             VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool        = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

}
