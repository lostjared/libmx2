#include "vk_model.hpp"

namespace mx {

    struct Vec2 { float x, y; };
    struct Vec3 { float x, y, z; };

    static bool ends_with(const std::string &s, const std::string &suf) {
        return s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
    }

    void MXModel::compressIndices() {
        if (vertices_.empty()) return;
        std::vector<VKVertex> unique;
        std::unordered_map<VKVertex, uint32_t, VKVertexHash> map;
        std::vector<uint32_t> newIdx;
        newIdx.reserve(vertices_.size());

        for (auto &v : vertices_) {
            auto it = map.find(v);
            if (it != map.end()) {
                newIdx.push_back(it->second);
            } else {
                uint32_t idx = static_cast<uint32_t>(unique.size());
                unique.push_back(v);
                map[v] = idx;
                newIdx.push_back(idx);
            }
        }

        size_t before = vertices_.size();
        vertices_ = std::move(unique);
        indices_ = std::move(newIdx);
        std::cout << ">> MXModel::compressIndices: " << before << " verts -> "
                  << vertices_.size() << " unique, " << indices_.size() << " indices\n";
    }

    void MXModel::load(const std::string &path, float positionScale) {
        std::string text;
        if (ends_with(path, ".mxmod.z")) {
            auto buffer = mx::readFile(path);
            if (buffer.empty())
                throw mx::Exception("MXModel::load: failed to read file: " + path);
            text = mx::decompressString(buffer.data(), static_cast<unsigned long>(buffer.size()));
        } else {
            std::ifstream f(path);
            if (!f.is_open())
                throw mx::Exception("MXModel::load: failed to open file: " + path);
            std::ostringstream ss;
            ss << f.rdbuf();
            text = ss.str();
        }

        
        if (text.size() >= 3 &&
            (unsigned char)text[0] == 0xEF &&
            (unsigned char)text[1] == 0xBB &&
            (unsigned char)text[2] == 0xBF) {
            text.erase(0, 3);
        }

        std::istringstream file(text);
        auto trim = [](std::string &s) {
            size_t b = s.find_first_not_of(" \t\r\n");
            if (b == std::string::npos) { s.clear(); return; }
            size_t e = s.find_last_not_of(" \t\r\n");
            s = s.substr(b, e - b + 1);
        };

        std::vector<Vec3> pos;
        std::vector<Vec2> uv;
        std::vector<Vec3> nrm;
        std::vector<uint32_t> fileIndices;

        int type = -1;
        size_t count = 0;
        std::string line;

        while (std::getline(file, line)) {
            if (line.empty()) continue;
            size_t commentPos = line.find('#');
            if (commentPos != std::string::npos) line = line.substr(0, commentPos);
            trim(line);
            if (line.empty()) continue;

            
            if (line.size() >= 3 &&
                (unsigned char)line[0] == 0xEF &&
                (unsigned char)line[1] == 0xBB &&
                (unsigned char)line[2] == 0xBF) {
                line.erase(0, 3);
                trim(line);
                if (line.empty()) continue;
            }

            std::istringstream s(line);

            
            char c = line[line.find_first_not_of(" \t")];
            bool isData = (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.';

            if (isData) {
                float x = 0, y = 0, z = 0;
                switch (type) {
                    case 0: 
                        if (s >> x >> y >> z)
                            pos.push_back({x * positionScale, y * positionScale, z * positionScale});
                        break;
                    case 1: 
                        if (s >> x >> y)
                            uv.push_back({x, y});
                        break;
                    case 2: 
                        if (s >> x >> y >> z)
                            nrm.push_back({x, y, z});
                        break;
                    case 5: { 
                        uint32_t idx;
                        while (s >> idx) fileIndices.push_back(idx);
                    } break;
                    default:
                        break;
                }
                continue;
            }

            std::string tag;
            s >> tag;
            if (tag.empty()) continue;

            if (tag == "tri") {
                uint32_t st = 0, ti = 0;
                s >> st >> ti;
                textureIndex_ = ti;
                type = -1;
                continue;
            }
            if (tag == "vert") { s >> count; type = 0; continue; }
            if (tag == "tex")  { s >> count; type = 1; continue; }
            if (tag == "norm") { s >> count; type = 2; continue; }
            if (tag == "indices") { s >> count; type = 5; continue; }
        }

        if (pos.empty())
            throw mx::Exception("MXModel::load: no vertices found in " + path);

        
        int vcount = static_cast<int>(pos.size());
        vertices_.clear();
        vertices_.reserve(vcount);

        for (int i = 0; i < vcount; ++i) {
            VKVertex v{};
            v.pos[0] = pos[i].x;
            v.pos[1] = pos[i].y;
            v.pos[2] = pos[i].z;
            if (i < (int)uv.size()) {
                v.texCoord[0] = uv[i].x;
                v.texCoord[1] = uv[i].y;
            }
            if (i < (int)nrm.size()) {
                v.normal[0] = nrm[i].x;
                v.normal[1] = nrm[i].y;
                v.normal[2] = nrm[i].z;
            }
            vertices_.push_back(v);
        }

        if (!fileIndices.empty()) {
            indices_ = std::move(fileIndices);
            std::cout << ">> MXModel: loaded " << vcount << " verts, " << indices_.size() << " indices from file\n";
        } else {
            indices_.clear();
            indices_.reserve(vcount);
            for (uint32_t i = 0; i < static_cast<uint32_t>(vcount); ++i)
                indices_.push_back(i);
            compressIndices();
        }
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
