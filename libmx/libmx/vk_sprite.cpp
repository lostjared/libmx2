#include"vk_sprite.hpp"
#include"loadpng.hpp"

namespace mx {

    VKSprite::VKSprite(VkDevice dev, VkPhysicalDevice physDev, VkQueue gQueue, VkCommandPool cmdPool)
        : device(dev), physicalDevice(physDev), graphicsQueue(gQueue), commandPool(cmdPool) {
    }
    
    VKSprite::~VKSprite() {
        vkDeviceWaitIdle(device);
        drawQueue.clear();
        
        destroyStagingResources();
        
        if (quadVertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, quadVertexBuffer, nullptr);
            vkFreeMemory(device, quadVertexBufferMemory, nullptr);
        }
        if (quadIndexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, quadIndexBuffer, nullptr);
            vkFreeMemory(device, quadIndexBufferMemory, nullptr);
        }
        
        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        }
        
        if (spriteSampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, spriteSampler, nullptr);
        }
        
        if (spriteImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, spriteImageView, nullptr);
        }
        
        if (spriteImage != VK_NULL_HANDLE) {
            vkDestroyImage(device, spriteImage, nullptr);
            vkFreeMemory(device, spriteImageMemory, nullptr);
        }
        
        if (fragmentShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
        }
        
        if (customPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, customPipeline, nullptr);
        }
        
        if (customPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, customPipelineLayout, nullptr);
        }
    }

    void VKSprite::destroySpriteResources() {
        destroyStagingResources();
        
        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            descriptorPool = VK_NULL_HANDLE;
            descriptorSet = VK_NULL_HANDLE;
        }

        if (spriteSampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, spriteSampler, nullptr);
            spriteSampler = VK_NULL_HANDLE;
        }

        if (spriteImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, spriteImageView, nullptr);
            spriteImageView = VK_NULL_HANDLE;
        }

        if (spriteImage != VK_NULL_HANDLE) {
            vkDestroyImage(device, spriteImage, nullptr);
            spriteImage = VK_NULL_HANDLE;
        }
        if (spriteImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, spriteImageMemory, nullptr);
            spriteImageMemory = VK_NULL_HANDLE;
        }

        if (fragmentShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
            fragmentShaderModule = VK_NULL_HANDLE;
        }

        if (customPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, customPipeline, nullptr);
            customPipeline = VK_NULL_HANDLE;
        }

        if (customPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, customPipelineLayout, nullptr);
            customPipelineLayout = VK_NULL_HANDLE;
        }

        hasCustomShader = false;
        spriteLoaded = false;
    }

    void VKSprite::createStagingResources(VkDeviceSize size) {
        if (stagingResourcesCreated && persistentStagingSize >= size) {
            return; 
        }
        destroyStagingResources();
        
        createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    persistentStagingBuffer, persistentStagingMemory);
        
        try {
            VK_CHECK_RESULT(vkMapMemory(device, persistentStagingMemory, 0, size, 0, &persistentStagingMapped));
            persistentStagingSize = size;
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = commandPool;
            allocInfo.commandBufferCount = 1;
            VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, &uploadCmdBuffer));
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; 
            VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &uploadFence));
            stagingResourcesCreated = true;
        } catch (...) {
            if (uploadCmdBuffer != VK_NULL_HANDLE) {
                vkFreeCommandBuffers(device, commandPool, 1, &uploadCmdBuffer);
                uploadCmdBuffer = VK_NULL_HANDLE;
            }
            if (persistentStagingMapped) {
                vkUnmapMemory(device, persistentStagingMemory);
                persistentStagingMapped = nullptr;
            }
            if (persistentStagingBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, persistentStagingBuffer, nullptr);
                persistentStagingBuffer = VK_NULL_HANDLE;
            }
            if (persistentStagingMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device, persistentStagingMemory, nullptr);
                persistentStagingMemory = VK_NULL_HANDLE;
            }
            persistentStagingSize = 0;
            throw;
        }
    }

    void VKSprite::destroyStagingResources() {
        if (!stagingResourcesCreated) return;
        
        if (uploadFence != VK_NULL_HANDLE) {
            vkWaitForFences(device, 1, &uploadFence, VK_TRUE, UINT64_MAX);
            vkDestroyFence(device, uploadFence, nullptr);
            uploadFence = VK_NULL_HANDLE;
        }
        if (uploadCmdBuffer != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(device, commandPool, 1, &uploadCmdBuffer);
            uploadCmdBuffer = VK_NULL_HANDLE;
        }
        if (persistentStagingBuffer != VK_NULL_HANDLE) {
            vkUnmapMemory(device, persistentStagingMemory);
            vkDestroyBuffer(device, persistentStagingBuffer, nullptr);
            vkFreeMemory(device, persistentStagingMemory, nullptr);
            persistentStagingBuffer = VK_NULL_HANDLE;
            persistentStagingMemory = VK_NULL_HANDLE;
            persistentStagingMapped = nullptr;
            persistentStagingSize = 0;
        }
        stagingResourcesCreated = false;
    }
    
    void VKSprite::createCustomPipeline() {
        if (!hasCustomShader || fragmentShaderModule == VK_NULL_HANDLE) return;
        if (renderPass == VK_NULL_HANDLE || descriptorSetLayout == VK_NULL_HANDLE) return;

        if (customPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, customPipeline, nullptr);
            customPipeline = VK_NULL_HANDLE;
        }
        if (customPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, customPipelineLayout, nullptr);
            customPipelineLayout = VK_NULL_HANDLE;
        }
        
        std::string vertPath = vertexShaderPath.empty() ? "data/sprite_vert.spv" : vertexShaderPath;
        auto vertShaderCode = readShaderFile(vertPath);
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragmentShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(float) * 4;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = 0;
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = sizeof(float) * 2;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(float) * 12;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &customPipelineLayout));

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = customPipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &customPipeline));

        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void VKSprite::createQuadBuffer() {
        if (quadBufferCreated) return;
        
        SpriteVertex vertices[] = {
            {{0.0f, 0.0f}, {0.0f, 0.0f}},
            {{1.0f, 0.0f}, {1.0f, 0.0f}},
            {{1.0f, 1.0f}, {1.0f, 1.0f}},
            {{0.0f, 1.0f}, {0.0f, 1.0f}}
        };
        uint16_t indices[] = {0, 1, 2, 0, 2, 3};
        
        VkDeviceSize vertexSize = sizeof(vertices);
        createBuffer(vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    quadVertexBuffer, quadVertexBufferMemory);
        
        void *data;
        VK_CHECK_RESULT(vkMapMemory(device, quadVertexBufferMemory, 0, vertexSize, 0, &data));
        memcpy(data, vertices, vertexSize);
        vkUnmapMemory(device, quadVertexBufferMemory);
        
        VkDeviceSize indexSize = sizeof(indices);
        createBuffer(indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    quadIndexBuffer, quadIndexBufferMemory);
        
        VK_CHECK_RESULT(vkMapMemory(device, quadIndexBufferMemory, 0, indexSize, 0, &data));
        memcpy(data, indices, indexSize);
        vkUnmapMemory(device, quadIndexBufferMemory);
        
        quadBufferCreated = true;
    }
    
    void VKSprite::loadSprite(const std::string &pngPath, const std::string &fragmentShaderPath) {
        SDL_Surface* surface = png::LoadPNG(pngPath.c_str());
        if (!surface) {
            throw mx::Exception("Failed to load sprite image: " + pngPath);
        }
        loadSprite(surface, fragmentShaderPath);
        SDL_FreeSurface(surface);
    }
    
    void VKSprite::loadSprite(SDL_Surface* surface, const std::string &fragmentShaderPath) {
        if (!surface) {
            throw mx::Exception("VKSprite::loadSprite called with null surface");
        }
        if (spriteLoaded || spriteImage != VK_NULL_HANDLE || fragmentShaderModule != VK_NULL_HANDLE) {
            destroySpriteResources();
        }
        SDL_Surface* rgbaSurface = convertToRGBA(surface);
        if (!rgbaSurface) {
            throw mx::Exception("Failed to convert sprite surface to RGBA");
        }
        spriteWidth = rgbaSurface->w;
        spriteHeight = rgbaSurface->h;
        createSpriteTexture(rgbaSurface);
        SDL_FreeSurface(rgbaSurface);    
        createSampler();
        createQuadBuffer();  
        if (!fragmentShaderPath.empty()) {
            auto shaderCode = readShaderFile(fragmentShaderPath);
            fragmentShaderModule = createShaderModule(shaderCode);
            hasCustomShader = true;
            
            if (renderPass != VK_NULL_HANDLE && descriptorSetLayout != VK_NULL_HANDLE) {
                createCustomPipeline();
            }
        }
        spriteLoaded = true;
    }

    void VKSprite::createEmptySprite(int width, int height, const std::string &vertexShaderPath, const std::string &fragmentShaderPath) {
        if (width <= 0 || height <= 0) {
            throw mx::Exception("VKSprite::createEmptySprite invalid dimensions");
        }
        if (spriteLoaded || spriteImage != VK_NULL_HANDLE || fragmentShaderModule != VK_NULL_HANDLE) {
            destroySpriteResources();
        }
        spriteWidth = width;
        spriteHeight = height;
        
        if (!vertexShaderPath.empty()) {
            setVertexShaderPath(vertexShaderPath);
        }
        
        createImage(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                   VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, spriteImage, spriteImageMemory);
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;
        
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    stagingBuffer, stagingMemory);
        
        void *data;
        VK_CHECK_RESULT(vkMapMemory(device, stagingMemory, 0, imageSize, 0, &data));
        memset(data, 0, imageSize);
        vkUnmapMemory(device, stagingMemory);
        
        transitionImageLayout(spriteImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, spriteImage, width, height);
        transitionImageLayout(spriteImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        
        spriteImageView = createImageView(spriteImage, VK_FORMAT_R8G8B8A8_UNORM);
        createSampler();
        createQuadBuffer();
        createDescriptorPool();
    
        createStagingResources(imageSize);
        
        if (!fragmentShaderPath.empty()) {
            auto shaderCode = readShaderFile(fragmentShaderPath);
            fragmentShaderModule = createShaderModule(shaderCode);
            hasCustomShader = true;
            
            if (renderPass != VK_NULL_HANDLE && descriptorSetLayout != VK_NULL_HANDLE) {
                createCustomPipeline();
            }
        }
        
        spriteLoaded = true;
    }

    void VKSprite::updateTexture(SDL_Surface* surface) {
        if (!surface) {
            throw mx::Exception("VKSprite::updateTexture called with null surface");
        }
        if (!spriteLoaded) {
            throw mx::Exception("VKSprite::updateTexture called before sprite was loaded");
        }
        SDL_Surface* rgbaSurface = convertToRGBA(surface);
        if (!rgbaSurface) {
            throw mx::Exception("Failed to convert surface to RGBA in updateTexture");
        }
        if (rgbaSurface->w == spriteWidth && rgbaSurface->h == spriteHeight) {
            updateSpriteTexture(rgbaSurface->pixels, rgbaSurface->w, rgbaSurface->h);
        } else {
            if (stagingResourcesCreated && uploadFence != VK_NULL_HANDLE) {
                vkWaitForFences(device, 1, &uploadFence, VK_TRUE, UINT64_MAX);
            }
            if (spriteImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device, spriteImageView, nullptr);
                spriteImageView = VK_NULL_HANDLE;
            }
            if (spriteImage != VK_NULL_HANDLE) {
                vkDestroyImage(device, spriteImage, nullptr);
                spriteImage = VK_NULL_HANDLE;
            }
            if (spriteImageMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device, spriteImageMemory, nullptr);
                spriteImageMemory = VK_NULL_HANDLE;
            }
            spriteWidth = rgbaSurface->w;
            spriteHeight = rgbaSurface->h;
            createSpriteTexture(rgbaSurface);
            if (descriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(device, descriptorPool, nullptr);
                descriptorPool = VK_NULL_HANDLE;
                descriptorSet = VK_NULL_HANDLE;
            }
            createDescriptorPool();
        }
        SDL_FreeSurface(rgbaSurface);
    }

    void VKSprite::updateTexture(const void* pixels, int width, int height, int pitch) {
        if (!pixels) {
            throw mx::Exception("VKSprite::updateTexture called with null pixel data");
        }
        if (!spriteLoaded) {
            throw mx::Exception("VKSprite::updateTexture called before sprite was loaded");
        }
        if (width <= 0 || height <= 0) {
            throw mx::Exception("VKSprite::updateTexture invalid dimensions");
        }
        int srcPitch = (pitch > 0) ? pitch : width * 4;
        if (width == spriteWidth && height == spriteHeight && srcPitch == width * 4) {
            updateSpriteTexture(pixels, width, height);
        } else if (width == spriteWidth && height == spriteHeight) {
            std::vector<uint8_t> packed(width * height * 4);
            const uint8_t* src = static_cast<const uint8_t*>(pixels);
            for (int row = 0; row < height; ++row) {
                memcpy(packed.data() + row * width * 4, src + row * srcPitch, width * 4);
            }
            updateSpriteTexture(packed.data(), width, height);
        } else {
            if (stagingResourcesCreated && uploadFence != VK_NULL_HANDLE) {
                vkWaitForFences(device, 1, &uploadFence, VK_TRUE, UINT64_MAX);
            }
            if (spriteImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device, spriteImageView, nullptr);
                spriteImageView = VK_NULL_HANDLE;
            }
            if (spriteImage != VK_NULL_HANDLE) {
                vkDestroyImage(device, spriteImage, nullptr);
                spriteImage = VK_NULL_HANDLE;
            }
            if (spriteImageMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device, spriteImageMemory, nullptr);
                spriteImageMemory = VK_NULL_HANDLE;
            }
            spriteWidth = width;
            spriteHeight = height;
            std::vector<uint8_t> packed;
            const void* texData = pixels;
            if (srcPitch != width * 4) {
                packed.resize(width * height * 4);
                const uint8_t* src = static_cast<const uint8_t*>(pixels);
                for (int row = 0; row < height; ++row) {
                    memcpy(packed.data() + row * width * 4, src + row * srcPitch, width * 4);
                }
                texData = packed.data();
            }
            SDL_Surface* tmpSurface = SDL_CreateRGBSurfaceFrom(
                const_cast<void*>(texData), width, height, 32, width * 4,
                0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
            if (!tmpSurface) {
                throw mx::Exception("VKSprite::updateTexture failed to create temp surface");
            }
            createSpriteTexture(tmpSurface);
            SDL_FreeSurface(tmpSurface);
            if (descriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(device, descriptorPool, nullptr);
                descriptorPool = VK_NULL_HANDLE;
                descriptorSet = VK_NULL_HANDLE;
            }
            createDescriptorPool();
        }
    }

    void VKSprite::updateSpriteTexture(const void* pixels, uint32_t width, uint32_t height) {
        VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;
    
        createStagingResources(imageSize);
        VK_CHECK_RESULT(vkWaitForFences(device, 1, &uploadFence, VK_TRUE, UINT64_MAX));
        VK_CHECK_RESULT(vkResetFences(device, 1, &uploadFence));
        memcpy(persistentStagingMapped, pixels, imageSize);
        VK_CHECK_RESULT(vkResetCommandBuffer(uploadCmdBuffer, 0));
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK_RESULT(vkBeginCommandBuffer(uploadCmdBuffer, &beginInfo));
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = spriteImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(uploadCmdBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &barrier);
    
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
        vkCmdCopyBufferToImage(uploadCmdBuffer, persistentStagingBuffer, spriteImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(uploadCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        VK_CHECK_RESULT(vkEndCommandBuffer(uploadCmdBuffer));
        
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &uploadCmdBuffer;
        VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, uploadFence));
    }

    void VKSprite::createSpriteTexture(SDL_Surface* surface) {
        createImage(surface->w, surface->h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                   VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, spriteImage, spriteImageMemory);
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        VkDeviceSize imageSize = static_cast<VkDeviceSize>(surface->w) * surface->h * 4;
        
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    stagingBuffer, stagingMemory);
        
        void *data;
        VK_CHECK_RESULT(vkMapMemory(device, stagingMemory, 0, imageSize, 0, &data));
        memcpy(data, surface->pixels, imageSize);
        vkUnmapMemory(device, stagingMemory);
        
        transitionImageLayout(spriteImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, spriteImage, surface->w, surface->h);
        transitionImageLayout(spriteImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        
        spriteImageView = createImageView(spriteImage, VK_FORMAT_R8G8B8A8_UNORM);
    }
    
    void VKSprite::createSampler() {
        if (spriteSampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, spriteSampler, nullptr);
            spriteSampler = VK_NULL_HANDLE;
        }
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        samplerInfo.mipLodBias = 0.0f;
        
        VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &spriteSampler));
    }
    
    void VKSprite::drawSprite(int x, int y) {
        drawSpriteRect(x, y, spriteWidth, spriteHeight);
    }
    
    void VKSprite::drawSprite(int x, int y, float scaleX, float scaleY) {
        drawSpriteRect(x, y, static_cast<int>(spriteWidth * scaleX), static_cast<int>(spriteHeight * scaleY));
    }
    
    void VKSprite::drawSprite(int x, int y, float scaleX, float scaleY, float rotation) {
        drawSpriteRect(x, y, static_cast<int>(spriteWidth * scaleX), static_cast<int>(spriteHeight * scaleY));
    }
    
    void VKSprite::drawSpriteRect(int x, int y, int w, int h) {
        if (!spriteLoaded) {
            throw mx::Exception("VKSprite::drawSpriteRect called before sprite was loaded");
        }
        
        drawQueue.push_back({static_cast<float>(x), static_cast<float>(y), 
                            static_cast<float>(w), static_cast<float>(h), shaderParams});
    }
    
    void VKSprite::setShaderParams(float p1, float p2, float p3, float p4) {
        shaderParams = glm::vec4(p1, p2, p3, p4);
    }
    
    void VKSprite::renderSprites(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout,
                                 uint32_t screenWidth, uint32_t screenHeight) {
        if (drawQueue.empty() || !spriteLoaded || !quadBufferCreated) return;
        if (descriptorSet == VK_NULL_HANDLE) {
            descriptorSet = createDescriptorSet(spriteImageView);
        }
        VkPipelineLayout layoutToUse = (customPipeline != VK_NULL_HANDLE) ? customPipelineLayout : pipelineLayout;
        if (customPipeline != VK_NULL_HANDLE) {
            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, customPipeline);
        }
        
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layoutToUse,
                               0, 1, &descriptorSet, 0, nullptr);
        
        VkBuffer vertexBuffers[] = {quadVertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmdBuffer, quadIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
        
        for (const auto &cmd : drawQueue) {
            struct SpritePushConstants {
                float screenWidth;
                float screenHeight;
                float spritePosX;
                float spritePosY;
                float spriteSizeW;
                float spriteSizeH;
                float padding1; 
                float padding2;
                float params[4];
            } pc {
                (float)screenWidth, (float)screenHeight,
                cmd.x, cmd.y, cmd.w, cmd.h,
                0.0f, 0.0f, 
                {cmd.params.x, cmd.params.y, cmd.params.z, cmd.params.w}
            };
            
            vkCmdPushConstants(cmdBuffer, layoutToUse, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                              0, sizeof(SpritePushConstants), &pc);
            
            vkCmdDrawIndexed(cmdBuffer, 6, 1, 0, 0, 0);
        }        
    }
    
    void VKSprite::clearQueue() {
        drawQueue.clear();  
    }
    
    void VKSprite::createDescriptorPool() {
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
    
    VkDescriptorSet VKSprite::createDescriptorSet(VkImageView imageView) {
        if (descriptorSetLayout == VK_NULL_HANDLE) {
            throw mx::Exception("VKSprite::createDescriptorSet called before setDescriptorSetLayout");
        }
        
        if (descriptorPool == VK_NULL_HANDLE) {
            createDescriptorPool();
        }
        
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        
        VkDescriptorSet descSet;
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descSet));
        
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageView;
        imageInfo.sampler = spriteSampler;
        
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;
        
        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        
        return descSet;
    }
    
    void VKSprite::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
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
    
    uint32_t VKSprite::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        
        throw mx::Exception("Failed to find suitable memory type!");
    }
    
    void VKSprite::transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
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
        } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }
        
        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        endSingleTimeCommands(commandBuffer);
    }
    
    void VKSprite::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
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
    
    VkCommandBuffer VKSprite::beginSingleTimeCommands() {
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
    
    void VKSprite::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
        VK_CHECK_RESULT(vkQueueWaitIdle(graphicsQueue));
        
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
    
    void VKSprite::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
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
    
    VkImageView VKSprite::createImageView(VkImage image, VkFormat format) {
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
    
    SDL_Surface* VKSprite::convertToRGBA(SDL_Surface* surface) {
        SDL_Surface* converted = SDL_CreateRGBSurface(0, surface->w, surface->h, 32,
                                                       0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
        if (!converted) return nullptr;
        SDL_BlitSurface(surface, nullptr, converted, nullptr);
        return converted;
    }
    
    std::vector<char> VKSprite::readShaderFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw mx::Exception("Failed to open shader file: " + filename);
        }
        
        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        
        return buffer;
    }
    
    VkShaderModule VKSprite::createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        
        VkShaderModule shaderModule;
        VK_CHECK_RESULT(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));
        return shaderModule;
    }

}
