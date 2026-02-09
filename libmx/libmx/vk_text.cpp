#include"vk_text.hpp"

namespace mx {

    VKText::VKText(VkDevice dev, VkPhysicalDevice physDev, VkQueue gQueue, 
                   VkCommandPool cmdPool, const std::string &fontPath, int fontSize)
        : device(dev), physicalDevice(physDev), graphicsQueue(gQueue), commandPool(cmdPool) {
        
        if (TTF_Init() == -1) {
            throw mx::Exception("Failed to initialize SDL_ttf: " + std::string(TTF_GetError()));
        }
        
        initFont(fontPath, fontSize);
    }
    
    VKText::~VKText() {
        vkDeviceWaitIdle(device);
        textQuads.clear();
        
        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        }
        
        if (fontSampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, fontSampler, nullptr);
        }
        
        if (font) {
            TTF_CloseFont(font);
        }
        TTF_Quit();
    }
    
    void VKText::createDescriptorPool() {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 100;
        
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 100;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        
        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));
    }
    
    VkDescriptorSet VKText::createDescriptorSet(VkImageView imageView) {
        if (descriptorSetLayout == VK_NULL_HANDLE) {
            throw mx::Exception("VKText::createDescriptorSet called before setDescriptorSetLayout");
        }
        
        if (descriptorPool == VK_NULL_HANDLE) {
            createDescriptorPool();
        }
        
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        
        VkDescriptorSet descriptorSet;
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
        
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageView;
        imageInfo.sampler = fontSampler;
        
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;
        
        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        
        return descriptorSet;
    }
    
    void VKText::initFont(const std::string &fontPath, int fontSize) {
        font = TTF_OpenFont(fontPath.c_str(), fontSize);
        if (!font) {
            throw mx::Exception("Failed to load font: " + std::string(TTF_GetError()));
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        
        VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &fontSampler));
    }

    void VKText::setFont(const std::string &fontPath, int fontSize) {
        vkDeviceWaitIdle(device);
        clearQueue();
        if (font) {
            TTF_CloseFont(font);
            font = nullptr;
        }
        if (fontSampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, fontSampler, nullptr);
            fontSampler = VK_NULL_HANDLE;
        }
        initFont(fontPath, fontSize);
    }
    
    SDL_Surface* VKText::convertToRGBA(SDL_Surface* surface) {
        SDL_Surface* converted = SDL_CreateRGBSurface(0, surface->w, surface->h, 32,
                                                       0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
        if (!converted) return nullptr;
        SDL_BlitSurface(surface, nullptr, converted, nullptr);
        return converted;
    }
    
    VkImageView VKText::createImageView(VkImage image, VkFormat format) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        VkImageView imageView;
        VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &imageView));
        return imageView;
    }
    
    void VKText::printTextG_Solid(const std::string &text, int x, int y, const SDL_Color &col) {
        if (text.empty() || !font) return;
        
        SDL_Surface *textSurface = TTF_RenderText_Blended(font, text.c_str(), col);
        if (!textSurface) return;
        
        SDL_Surface *rgbaSurface = convertToRGBA(textSurface);
        SDL_FreeSurface(textSurface);
        if (!rgbaSurface) return;
        
        TextQuad quad;
        quad.device = device;
        quad.text = text;
        quad.x = x;
        quad.y = y;
        quad.width = rgbaSurface->w;
        quad.height = rgbaSurface->h;
        quad.color = col;
        
        createImage(rgbaSurface->w, rgbaSurface->h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                   VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, quad.textImage, quad.textImageMemory);
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        VkDeviceSize imageSize = rgbaSurface->w * rgbaSurface->h * 4;
        
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    stagingBuffer, stagingMemory);
        
        void *data;
        VK_CHECK_RESULT(vkMapMemory(device, stagingMemory, 0, imageSize, 0, &data));
        memcpy(data, rgbaSurface->pixels, imageSize);
        vkUnmapMemory(device, stagingMemory);
        
        transitionImageLayout(quad.textImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, quad.textImage, rgbaSurface->w, rgbaSurface->h);
        transitionImageLayout(quad.textImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        
        quad.textImageView = createImageView(quad.textImage, VK_FORMAT_R8G8B8A8_UNORM);
        
        float x0 = (float)x;
        float y0 = (float)y;
        float x1 = x0 + rgbaSurface->w;
        float y1 = y0 + rgbaSurface->h;
        
        TextVertex v0 = {{x0, y0}, {0.0f, 0.0f}};
        TextVertex v1 = {{x1, y0}, {1.0f, 0.0f}};
        TextVertex v2 = {{x1, y1}, {1.0f, 1.0f}};
        TextVertex v3 = {{x0, y1}, {0.0f, 1.0f}};
        
        quad.vertices = {v0, v1, v2, v3};
        quad.indices = {0, 1, 2, 0, 2, 3};
        quad.indexCount = 6;
        
        VkDeviceSize vertexSize = quad.vertices.size() * sizeof(TextVertex);
        createBuffer(vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    quad.vertexBuffer, quad.vertexBufferMemory);
        
        VK_CHECK_RESULT(vkMapMemory(device, quad.vertexBufferMemory, 0, vertexSize, 0, &data));
        memcpy(data, quad.vertices.data(), vertexSize);
        vkUnmapMemory(device, quad.vertexBufferMemory);
        
        VkDeviceSize indexSize = quad.indices.size() * sizeof(uint16_t);
        createBuffer(indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    quad.indexBuffer, quad.indexBufferMemory);
        
        VK_CHECK_RESULT(vkMapMemory(device, quad.indexBufferMemory, 0, indexSize, 0, &data));
        memcpy(data, quad.indices.data(), indexSize);
        vkUnmapMemory(device, quad.indexBufferMemory);
        
        quad.descriptorSet = createDescriptorSet(quad.textImageView);
        
        SDL_FreeSurface(rgbaSurface);
        textQuads.emplace_back(std::move(quad));
    }
    
    void VKText::renderText(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout,
                           uint32_t screenWidth, uint32_t screenHeight) {
                        
        struct TextPushConstants {
            float screenWidth;
            float screenHeight;
        } pc { static_cast<float>(screenWidth), static_cast<float>(screenHeight) };

        vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TextPushConstants), &pc);

        for (auto &quad : textQuads) {
            VkBuffer vertexBuffers[] = {quad.vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(cmdBuffer, quad.indexBuffer, 0, VK_INDEX_TYPE_UINT16);
            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &quad.descriptorSet, 0, nullptr);
            
            vkCmdDrawIndexed(cmdBuffer, quad.indexCount, 1, 0, 0, 0);
        }
    }
    
    void VKText::clearQueue() {
        VK_CHECK_RESULT(vkQueueWaitIdle(graphicsQueue));
        textQuads.clear();
        if (descriptorPool != VK_NULL_HANDLE) {
            VK_CHECK_RESULT(vkResetDescriptorPool(device, descriptorPool, 0));
        }
    }
    
    void VKText::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags properties, VkBuffer& buffer,
                             VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));
        
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
        
        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory));
        VK_CHECK_RESULT(vkBindBufferMemory(device, buffer, bufferMemory, 0));
    }
    
    bool VKText::getTextDimensions(const std::string &text, int& width, int& height) {
        if (text.empty() || !font) {
            width = 0;
            height = 0;
            return false;
        }
        
        if (TTF_SizeText(font, text.c_str(), &width, &height) == 0) {
            return true;
        }
        return false;
    }
    
    uint32_t VKText::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        
        throw mx::Exception("Failed to find suitable memory type!");
    }
    
    void VKText::transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;
        
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }
        
        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        endSingleTimeCommands(commandBuffer);
    }
    
    void VKText::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};
        
        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        
        endSingleTimeCommands(commandBuffer);
    }
    
    VkCommandBuffer VKText::beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer commandBuffer;
        VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
        
        return commandBuffer;
    }
    
    void VKText::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
        VK_CHECK_RESULT(vkQueueWaitIdle(graphicsQueue));
        
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
    
    void VKText::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                            VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                            VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &image));
        
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
        
        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));
        VK_CHECK_RESULT(vkBindImageMemory(device, image, imageMemory, 0));
    }

}
