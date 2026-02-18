#include "vk.hpp"
#include "loadpng.hpp"

namespace mx {

    SkyboxViewer::SkyboxViewer(const std::string &title, int width, int height, bool full, const std::string& shaderDir) {
        initWindow(title, width, height, full);
        if (!shaderDir.empty()) {
            loadShaderIndex(shaderDir);
        }
    }

    void SkyboxViewer::initWindow(const std::string &title, int width, int height, bool full) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0)
            throw mx::Exception("SDL_Init: " + std::string(SDL_GetError()));
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            if (SDL_IsGameController(i)) {
                if (controller.open(i)) {
                    std::cout << ">> Game controller opened: " << controller.name() << "\n";
                    break;
                }
            }
        }
        Uint32 flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
        if (full) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
        if (!window) throw mx::Exception("SDL_CreateWindow: " + std::string(SDL_GetError()));
        w = width;
        h = height;
    }

    void SkyboxViewer::quit() { active = false; }

    void SkyboxViewer::setPath(const std::string &path) { util.path = path; }

    void SkyboxViewer::setShaderPath(const std::string &path) {
        if (!path.empty()) {
            loadShaderIndex(path);
        }
    }

    void SkyboxViewer::loadShaderIndex(const std::string& shaderDir) {
        shaderPath = shaderDir;
        std::string indexFile = shaderDir + "/index.txt";
        std::ifstream infile(indexFile);
        if (!infile.is_open()) {
            SDL_Log("mx: Could not open shader index file: %s", indexFile.c_str());
            return;
        }
        shaderFiles.clear();
        std::string line;
        while (std::getline(infile, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
                line.pop_back();
            while (!line.empty() && (line.front() == ' '))
                line.erase(line.begin());
            if (!line.empty()) {
                shaderFiles.push_back(line);
            }
        }
        infile.close();
        if (!shaderFiles.empty()) {
            useExternalShaders = true;
            effectIndex = 0;
            effects.clear();
            for (const auto& f : shaderFiles) {
                std::string name = f;
                auto dot = name.rfind('.');
                if (dot != std::string::npos)
                    name = name.substr(0, dot);
                effects.push_back(name);
            }
            SDL_Log("mx: Loaded %zu external shaders from %s", shaderFiles.size(), indexFile.c_str());
        }
    }

    void SkyboxViewer::openFile(const std::string &fn) {
        this->filename = fn;
        if(!cap.open(fn)) {
            throw mx::Exception("Error could not open file: " + fn);
        }
        camera_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        camera_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        useVideoTexture = true;
        SDL_Log("mx: Opened video file: %s (%dx%d)", fn.c_str(), camera_width, camera_height);
    }

    void SkyboxViewer::openCamera(int device, int width, int height) {
        if(!cap.open(device)) {
            throw mx::Exception("Error could not open camera device: " + std::to_string(device));
        }
        cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
        camera_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        camera_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        useVideoTexture = true;
        SDL_Log("mx: Opened camera device %d (%dx%d)", device, camera_width, camera_height);
    }

    void SkyboxViewer::activateFirstShader() {
        if (useExternalShaders && !shaderFiles.empty()) {
            effectIndex = 0;
            swapFragmentShader(0);
            SDL_Log("mx: Activated first shader: %s", effects[0].c_str());
        }
    }


    void SkyboxViewer::initVulkan() {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createDepthResources();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        loadModelData();
        loadCubemapTexture();
        createCubemapSampler();
        createDescriptorPool();
        createUniformBuffers();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    void SkyboxViewer::loadModelData() {
        std::string path = util.getFilePath("cube.mxmod.z");
        model.load(path, 1.0f);
        model.upload(device, physicalDevice, commandPool, graphicsQueue);
        std::cout << ">> Loaded skybox model: " << path << " (" << model.indexCount() << " indices)\n";
    }

    void SkyboxViewer::loadCubemapTexture() {
        
        if (useVideoTexture) {
            if (camera_width > 0 && camera_height > 0) {
                setupTextureImage(camera_width, camera_height);
                createTextureImageView();
            }
            return;
        }
        
        std::vector<std::string> faces = {
            util.getFilePath("ACMX2.Snapshot-2026.01.28-02.41.41-1280x720-0.png"),
            util.getFilePath("ACMX2.Snapshot-2026.01.28-02.41.43-1280x720-1.png"),
            util.getFilePath("ACMX2.Snapshot-2026.01.28-02.41.44-1280x720-2.png"),
            util.getFilePath("ACMX2.Snapshot-2026.01.28-02.41.48-1280x720-3.png"),
            util.getFilePath("ACMX2.Snapshot-2026.01.28-02.41.55-1280x720-4.png"),
            util.getFilePath("ACMX2.Snapshot-2026.01.28-02.41.58-1280x720-5.png")
        };

        
        SDL_Surface *firstFace = png::LoadPNG(faces[0].c_str());
        if (!firstFace) throw mx::Exception("Failed to load cubemap face: " + faces[0]);
        uint32_t faceW = firstFace->w;
        uint32_t faceH = firstFace->h;
        SDL_FreeSurface(firstFace);

        VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
        VkDeviceSize faceSize = faceW * faceH * 4;

        
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {faceW, faceH, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6;
        imageInfo.format = fmt;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        if (vkCreateImage(device, &imageInfo, nullptr, &cubemapImage) != VK_SUCCESS)
            throw mx::Exception("Failed to create cubemap image!");

        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(device, cubemapImage, &memReq);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkAllocateMemory(device, &allocInfo, nullptr, &cubemapImageMemory) != VK_SUCCESS)
            throw mx::Exception("Failed to allocate cubemap memory!");
        vkBindImageMemory(device, cubemapImage, cubemapImageMemory, 0);

        
        transitionImageLayout(cubemapImage, fmt, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6);

        
        for (uint32_t i = 0; i < 6; i++) {
            SDL_Surface *img = png::LoadPNG(faces[i].c_str());
            if (!img) throw mx::Exception("Failed to load cubemap face: " + faces[i]);

            VkBuffer staging;
            VkDeviceMemory stagingMem;
            createBuffer(faceSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging, stagingMem);
            void *data;
            vkMapMemory(device, stagingMem, 0, faceSize, 0, &data);
            memcpy(data, img->pixels, (size_t)faceSize);
            vkUnmapMemory(device, stagingMem);

            copyBufferToImage(staging, cubemapImage, faceW, faceH, i);

            vkDestroyBuffer(device, staging, nullptr);
            vkFreeMemory(device, stagingMem, nullptr);
            SDL_FreeSurface(img);

            std::cout << ">> Cubemap face[" << i << "]: " << faces[i] << " (" << faceW << "x" << faceH << ")\n";
        }

        
        transitionImageLayout(cubemapImage, fmt, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);

        
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = cubemapImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = fmt;
        viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6;

        if (vkCreateImageView(device, &viewInfo, nullptr, &cubemapImageView) != VK_SUCCESS)
            throw mx::Exception("Failed to create cubemap image view!");

        std::cout << ">> Cubemap texture loaded successfully\n";
    }

    void SkyboxViewer::createCubemapSampler() {
        VkSamplerCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        ci.magFilter = VK_FILTER_LINEAR;
        ci.minFilter = VK_FILTER_LINEAR;
        ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        ci.anisotropyEnable = VK_TRUE;
        ci.maxAnisotropy = 16.0f;
        ci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        
        if (useVideoTexture) {
            if (vkCreateSampler(device, &ci, nullptr, &textureSampler) != VK_SUCCESS)
                throw mx::Exception("Failed to create texture sampler!");
        } else {
            if (vkCreateSampler(device, &ci, nullptr, &cubemapSampler) != VK_SUCCESS)
                throw mx::Exception("Failed to create cubemap sampler!");
        }
    }

    void SkyboxViewer::setupTextureImage(int width, int height) {
        VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
        
        
        if (textureImage != VK_NULL_HANDLE) {
            vkDestroyImage(device, textureImage, nullptr);
            textureImage = VK_NULL_HANDLE;
        }
        if (textureImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, textureImageMemory, nullptr);
            textureImageMemory = VK_NULL_HANDLE;
        }
        
        VkImageCreateInfo imgInfo{};
        imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgInfo.imageType = VK_IMAGE_TYPE_2D;
        imgInfo.extent.width = static_cast<uint32_t>(width);
        imgInfo.extent.height = static_cast<uint32_t>(height);
        imgInfo.extent.depth = 1;
        imgInfo.mipLevels = 1;
        imgInfo.arrayLayers = 1;
        imgInfo.format = fmt;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        if (vkCreateImage(device, &imgInfo, nullptr, &textureImage) != VK_SUCCESS)
            throw mx::Exception("Failed to create texture image!");
        
        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(device, textureImage, &memReq);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        
        if (vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS)
            throw mx::Exception("Failed to allocate texture memory!");
        
        vkBindImageMemory(device, textureImage, textureImageMemory, 0);
        
        
        transitionImageLayout(textureImage, fmt, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
        transitionImageLayout(textureImage, fmt, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
        
        SDL_Log("mx: Created texture image (%dx%d)", width, height);
    }

    void SkyboxViewer::createTextureImageView() {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = textureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS)
            throw mx::Exception("Failed to create texture image view!");
    }

    void SkyboxViewer::updateTextureFromFrame(const cv::Mat& frame) {
        if (textureImage == VK_NULL_HANDLE || frame.empty()) return;
        
        cv::Mat rgba;
        if (frame.channels() == 3) {
            cv::cvtColor(frame, rgba, cv::COLOR_BGR2RGBA);
        } else if (frame.channels() == 4) {
            rgba = frame.clone();
        } else {
            return;
        }
        
        VkDeviceSize imageSize = rgba.total() * rgba.elemSize();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);
        
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, rgba.data, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);
        
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
        
        copyBufferToImage(stagingBuffer, textureImage,
                          static_cast<uint32_t>(rgba.cols),
                          static_cast<uint32_t>(rgba.rows), 0);
        
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
        
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void SkyboxViewer::updateExtFragDescriptors() {
        if (!useExternalShaders || extFragDescSets.empty()) return;
        
        uint32_t imgCount = static_cast<uint32_t>(swapChainImages.size());
        for (uint32_t i = 0; i < imgCount; i++) {
            VkDescriptorImageInfo imgInfo{};
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imgInfo.imageView = useVideoTexture ? textureImageView : cubemapImageView;
            imgInfo.sampler = useVideoTexture ? textureSampler : cubemapSampler;
            
            VkDescriptorBufferInfo bufInfo{};
            bufInfo.buffer = extSpriteBuffers[i];
            bufInfo.offset = 0;
            bufInfo.range = sizeof(SpriteExtendedUBO);
            
            std::array<VkWriteDescriptorSet, 2> writes{};
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = extFragDescSets[i];
            writes[0].dstBinding = 0;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[0].descriptorCount = 1;
            writes[0].pImageInfo = &imgInfo;
            
            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = extFragDescSets[i];
            writes[1].dstBinding = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[1].descriptorCount = 1;
            writes[1].pBufferInfo = &bufInfo;
            
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
        }
    }

    void SkyboxViewer::swapFragmentShader(size_t shaderIndex) {
        if (shaderIndex >= shaderFiles.size()) return;
        
        std::string spvPath = shaderPath + "/" + shaderFiles[shaderIndex];
        
        std::ifstream file(spvPath, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            SDL_Log("mx: Shader file not found: %s", spvPath.c_str());
            return;
        }
        file.close();
        
        vkDeviceWaitIdle(device);
        
        if (graphicsPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, graphicsPipeline, nullptr);
            graphicsPipeline = VK_NULL_HANDLE;
        }
        
        
        if (extPipelineLayout == VK_NULL_HANDLE) {
            {
                std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
                bindings[0].binding = 0;
                bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                bindings[0].descriptorCount = 1;
                bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                
                bindings[1].binding = 1;
                bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                bindings[1].descriptorCount = 1;
                bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                
                VkDescriptorSetLayoutCreateInfo ci{};
                ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                ci.bindingCount = static_cast<uint32_t>(bindings.size());
                ci.pBindings = bindings.data();
                VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &ci, nullptr, &extFragDescSetLayout));
            }
            
            {
                VkDescriptorSetLayoutBinding binding{};
                binding.binding = 0;
                binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                binding.descriptorCount = 1;
                binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                
                VkDescriptorSetLayoutCreateInfo ci{};
                ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                ci.bindingCount = 1;
                ci.pBindings = &binding;
                VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &ci, nullptr, &extVertDescSetLayout));
            }
            
            VkPushConstantRange pcRange{};
            pcRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            pcRange.offset = 0;
            pcRange.size = sizeof(ExtPushConstants);
            
            std::array<VkDescriptorSetLayout, 2> setLayouts = { extFragDescSetLayout, extVertDescSetLayout };
            VkPipelineLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
            layoutInfo.pSetLayouts = setLayouts.data();
            layoutInfo.pushConstantRangeCount = 1;
            layoutInfo.pPushConstantRanges = &pcRange;
            VK_CHECK_RESULT(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &extPipelineLayout));
            
            
            uint32_t imgCount = static_cast<uint32_t>(swapChainImages.size());
            std::array<VkDescriptorPoolSize, 2> poolSizes{};
            poolSizes[0] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imgCount };
            poolSizes[1] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, imgCount * 2 };
            
            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.maxSets = imgCount * 2;
            VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &extDescPool));
            
            extFragDescSets.resize(imgCount);
            {
                std::vector<VkDescriptorSetLayout> layouts(imgCount, extFragDescSetLayout);
                VkDescriptorSetAllocateInfo ai{};
                ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                ai.descriptorPool = extDescPool;
                ai.descriptorSetCount = imgCount;
                ai.pSetLayouts = layouts.data();
                VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &ai, extFragDescSets.data()));
            }
            
            extVertDescSets.resize(imgCount);
            {
                std::vector<VkDescriptorSetLayout> layouts(imgCount, extVertDescSetLayout);
                VkDescriptorSetAllocateInfo ai{};
                ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                ai.descriptorPool = extDescPool;
                ai.descriptorSetCount = imgCount;
                ai.pSetLayouts = layouts.data();
                VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &ai, extVertDescSets.data()));
            }
            
            extSpriteBuffers.resize(imgCount);
            extSpriteMemory.resize(imgCount);
            extSpriteMapped.resize(imgCount);
            
            for (uint32_t i = 0; i < imgCount; i++) {
                VkBufferCreateInfo bi{};
                bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bi.size = sizeof(SpriteExtendedUBO);
                bi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                VK_CHECK_RESULT(vkCreateBuffer(device, &bi, nullptr, &extSpriteBuffers[i]));
                
                VkMemoryRequirements memReq;
                vkGetBufferMemoryRequirements(device, extSpriteBuffers[i], &memReq);
                
                VkMemoryAllocateInfo mai{};
                mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                mai.allocationSize = memReq.size;
                mai.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                VK_CHECK_RESULT(vkAllocateMemory(device, &mai, nullptr, &extSpriteMemory[i]));
                vkBindBufferMemory(device, extSpriteBuffers[i], extSpriteMemory[i], 0);
                vkMapMemory(device, extSpriteMemory[i], 0, sizeof(SpriteExtendedUBO), 0, &extSpriteMapped[i]);
                
                SpriteExtendedUBO init{};
                init.u0 = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);
                memcpy(extSpriteMapped[i], &init, sizeof(init));
            }
            
            updateExtFragDescriptors();
            
            
            for (uint32_t i = 0; i < imgCount; i++) {
                VkDescriptorBufferInfo bufInfo{};
                bufInfo.buffer = uniformBuffers[i];
                bufInfo.offset = 0;
                bufInfo.range = sizeof(SkyboxUBO);
                
                VkWriteDescriptorSet write{};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.dstSet = extVertDescSets[i];
                write.dstBinding = 0;
                write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                write.descriptorCount = 1;
                write.pBufferInfo = &bufInfo;
                vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
            }
            
            SDL_Log("mx: Created external shader descriptor resources");
        }
        
        
        auto vertShaderCode = mx::readFile(util.getFilePath("ext_vert.spv"));
        auto fragShaderCode = mx::readFile(spvPath);
        
        VkShaderModule vertModule = createShaderModule(vertShaderCode);
        VkShaderModule fragModule = createShaderModule(fragShaderCode);
        
        if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) {
            SDL_Log("mx: Failed to create shader modules for: %s", spvPath.c_str());
            if (vertModule != VK_NULL_HANDLE) vkDestroyShaderModule(device, vertModule, nullptr);
            if (fragModule != VK_NULL_HANDLE) vkDestroyShaderModule(device, fragModule, nullptr);
            return;
        }
        
        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertModule;
        vertStage.pName = "main";
        
        VkPipelineShaderStageCreateInfo fragStage{};
        fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = fragModule;
        fragStage.pName = "main";
        
        VkPipelineShaderStageCreateInfo stages[] = { vertStage, fragStage };
        
        VkVertexInputBindingDescription bindDesc{};
        bindDesc.binding = 0;
        bindDesc.stride = sizeof(VKVertex);
        bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        std::array<VkVertexInputAttributeDescription, 3> attrDescs{};
        attrDescs[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VKVertex, pos) };
        attrDescs[1] = { 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VKVertex, texCoord) };
        attrDescs[2] = { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VKVertex, normal) };
        
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &bindDesc;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
        vertexInput.pVertexAttributeDescriptions = attrDescs.data();
        
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;
        
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
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
        
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = extPipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            SDL_Log("mx: Failed to create graphics pipeline!");
        }
        
        vkDestroyShaderModule(device, vertModule, nullptr);
        vkDestroyShaderModule(device, fragModule, nullptr);
        
        SDL_Log("mx: Loaded external shader: %s", effects[shaderIndex].c_str());
    }

    void SkyboxViewer::event(SDL_Event &e) {
        if (e.type == SDL_QUIT) { quit(); return; }
        if (controller.connectEvent(e)) {
            std::cout << ">> Controller connected: " << controller.name() << "\n";
        }
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE: quit(); break;
                case SDLK_LEFT:
                    if (useExternalShaders && !shaderFiles.empty()) {
                        effectIndex = (effectIndex - 1 + static_cast<int>(shaderFiles.size())) % static_cast<int>(shaderFiles.size());
                        swapFragmentShader(effectIndex);
                        SDL_Log("mx: Switched to shader: %s", effects[effectIndex].c_str());
                    }
                    break;
                case SDLK_RIGHT:
                    if (useExternalShaders && !shaderFiles.empty()) {
                        effectIndex = (effectIndex + 1) % static_cast<int>(shaderFiles.size());
                        swapFragmentShader(effectIndex);
                        SDL_Log("mx: Switched to shader: %s", effects[effectIndex].c_str());
                    }
                    break;
                case SDLK_m:
                    mouseControl = !mouseControl;
                    if (mouseControl) {
                        SDL_WarpMouseInWindow(window, w/2, h/2);
                        SDL_ShowCursor(SDL_DISABLE);
                        lastMouseX = w/2;
                        lastMouseY = h/2;
                    } else {
                        SDL_ShowCursor(SDL_ENABLE);
                    }
                    break;
                case SDLK_r:
                    fractalCenterX = -0.5;
                    fractalCenterY = 0.0;
                    fractalZoom = 1.0;
                    fractalMaxIter = 256;
                    break;
                case SDLK_1:
                    fractalCenterX = -0.745;
                    fractalCenterY = 0.113;
                    fractalZoom = 50.0;
                    break;
                case SDLK_2:
                    fractalCenterX = 0.275;
                    fractalCenterY = 0.0;
                    fractalZoom = 10.0;
                    break;
                case SDLK_3:
                    fractalCenterX = -0.761574;
                    fractalCenterY = -0.0847596;
                    fractalZoom = 200.0;
                    break;
                case SDLK_4:
                    fractalMaxIter += 25;
                    break;
            }
        }
        if (e.type == SDL_MOUSEMOTION && mouseControl) {
            int deltaX = e.motion.x - lastMouseX;
            int deltaY = e.motion.y - lastMouseY;
            yaw += deltaX * mouseSensitivity;
            pitch += deltaY * mouseSensitivity;
            pitch = glm::clamp(pitch, -89.0f, 89.0f);
            lastMouseX = e.motion.x;
            lastMouseY = e.motion.y;
            if (e.motion.x <= 10 || e.motion.x >= w - 10 ||
                e.motion.y <= 10 || e.motion.y >= h - 10) {
                SDL_WarpMouseInWindow(window, w/2, h/2);
                lastMouseX = w/2;
                lastMouseY = h/2;
            }
        }
    }

    void SkyboxViewer::loop() {
        SDL_Event e;
        while (active) {
            while (SDL_PollEvent(&e)) event(e);
            proc();
            draw();
        }
    }

    void SkyboxViewer::proc() {
        if (!useVideoTexture) return;
        
        cv::Mat frame;
        if (!cap.read(frame)) {
            if (!filename.empty()) {
                
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                if (!cap.read(frame)) {
                    quit();
                    return;
                }
            } else {
                
                quit();
                return;
            }
        }
        
        if (!frame.empty()) {
            updateTextureFromFrame(frame);
        }
    }

    void SkyboxViewer::draw() {
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) { recreateSwapChain(); return; }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw mx::Exception("Failed to acquire swap chain image!");

        vkResetCommandBuffer(commandBuffers[imageIndex], 0);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
            throw mx::Exception("Failed to begin recording command buffer!");

        VkRenderPassBeginInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass = renderPass;
        rpInfo.framebuffer = swapChainFramebuffers[imageIndex];
        rpInfo.renderArea.offset = {0, 0};
        rpInfo.renderArea.extent = swapChainExtent;
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        rpInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        rpInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[imageIndex], &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        
        static Uint32 lastTime = SDL_GetTicks();
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_A])
            yaw -= 60.0f * deltaTime;
        if (keystate[SDL_SCANCODE_D])
            yaw += 60.0f * deltaTime;
        if (keystate[SDL_SCANCODE_S])
            pitch += 60.0f * deltaTime;
        if (keystate[SDL_SCANCODE_W])
            pitch -= 60.0f * deltaTime;

        
        double panSpeed = 0.5 / fractalZoom * (double)deltaTime;
        if (keystate[SDL_SCANCODE_J])
            fractalCenterX -= panSpeed;
        if (keystate[SDL_SCANCODE_L])
            fractalCenterX += panSpeed;
        if (keystate[SDL_SCANCODE_I])
            fractalCenterY -= panSpeed;
        if (keystate[SDL_SCANCODE_K])
            fractalCenterY += panSpeed;
        if (keystate[SDL_SCANCODE_Z])
            fractalZoom *= 1.0 + 1.0 * (double)deltaTime;
        if (keystate[SDL_SCANCODE_X])
            fractalZoom *= 1.0 - 1.0 * (double)deltaTime;

        if (controller.active()) {
            const int deadzone = 8000;
            Sint16 axisX = controller.getAxis(SDL_CONTROLLER_AXIS_LEFTX);
            Sint16 axisY = controller.getAxis(SDL_CONTROLLER_AXIS_LEFTY);
            if (axisX < -deadzone)
                yaw -= 60.0f * deltaTime * (float)(-axisX) / 32767.0f;
            else if (axisX > deadzone)
                yaw += 60.0f * deltaTime * (float)(axisX) / 32767.0f;
            if (axisY < -deadzone)
                pitch -= 60.0f * deltaTime * (float)(-axisY) / 32767.0f;
            else if (axisY > deadzone)
                pitch += 60.0f * deltaTime * (float)(axisY) / 32767.0f;
        }

        pitch = glm::clamp(pitch, -89.0f, 89.0f);

        
        SkyboxUBO ubo{};
        float time = SDL_GetTicks() / 1000.0f;
        ubo.params = glm::vec4(time, (float)swapChainExtent.width, (float)swapChainExtent.height, 0.0f);

        ubo.model = glm::mat4(1.0f);

        glm::mat4 view = glm::mat4(1.0f);
        view = glm::rotate(view, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.view = view;

        float aspect = (float)swapChainExtent.width / (float)swapChainExtent.height;
        ubo.proj = glm::perspective(glm::radians(70.0f), aspect, 0.1f, 100.0f);
        ubo.proj[1][1] *= -1;

        if (uniformBuffersMapped.size() > imageIndex && uniformBuffersMapped[imageIndex])
            memcpy(uniformBuffersMapped[imageIndex], &ubo, sizeof(ubo));

        if (useExternalShaders && extPipelineLayout != VK_NULL_HANDLE) {
            
            VkDescriptorSet sets[] = { extFragDescSets[imageIndex], extVertDescSets[imageIndex] };
            vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    extPipelineLayout, 0, 2, sets, 0, nullptr);
            
            ExtPushConstants epc{};
            epc.screenSize = glm::vec2(static_cast<float>(swapChainExtent.width),
                                       static_cast<float>(swapChainExtent.height));
            epc.spritePos = glm::vec2(0.0f);
            epc.spriteSize = glm::vec2(static_cast<float>(swapChainExtent.width),
                                       static_cast<float>(swapChainExtent.height));
            epc.params = glm::vec4(1.0f, 1.0f, 1.0f, time);
            vkCmdPushConstants(commandBuffers[imageIndex], extPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, sizeof(ExtPushConstants), &epc);
            
            SpriteExtendedUBO extUBO{};
            extUBO.mouse = glm::vec4(0.0f);
            extUBO.u0 = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);
            extUBO.u1 = glm::vec4(0.016f, 0.0f, 0.0f, 60.0f);
            extUBO.u2 = glm::vec4(0.0f);
            extUBO.u3 = glm::vec4(0.0f);
            memcpy(extSpriteMapped[imageIndex], &extUBO, sizeof(extUBO));
        } else {
            
            if (!descriptorSets.empty())
                vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[imageIndex], 0, nullptr);

            FractalPushConstants pc{};
            pc.centerX = fractalCenterX;
            pc.centerY = fractalCenterY;
            pc.zoom = fractalZoom;
            pc.maxIterations = fractalMaxIter;
            pc.time = time;
            vkCmdPushConstants(commandBuffers[imageIndex], pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FractalPushConstants), &pc);
        }

        model.draw(commandBuffers[imageIndex]);
        vkCmdEndRenderPass(commandBuffers[imageIndex]);

        if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS)
            throw mx::Exception("Failed to record command buffer!");

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSems[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSems;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        VkSemaphore signalSems[] = {renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSems;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
            throw mx::Exception("Failed to submit draw command buffer!");

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSems;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) recreateSwapChain();
        else if (result != VK_SUCCESS) throw mx::Exception("Failed to present swap chain image!");
        vkQueueWaitIdle(presentQueue);
    }

    

    void SkyboxViewer::createInstance() {
#ifndef WITH_MOLTEN
        if (volkInitialize() != VK_SUCCESS) throw mx::Exception("Failed to initialize Volk!");
#endif
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Skybox Viewer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        unsigned int sdlExtCount = 0;
        SDL_Vulkan_GetInstanceExtensions(window, &sdlExtCount, nullptr);
        std::vector<const char*> extensions(sdlExtCount);
        SDL_Vulkan_GetInstanceExtensions(window, &sdlExtCount, extensions.data());

        const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#ifdef WITH_MOLTEN
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

        bool layersSupported = true;
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        for (const char* name : validationLayers) {
            bool found = false;
            for (const auto& l : availableLayers) if (strcmp(name, l.layerName) == 0) { found = true; break; }
            if (!found) { layersSupported = false; break; }
        }

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = (uint32_t)extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();
#ifdef WITH_MOLTEN
        createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
        if (layersSupported) {
            createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &instance));
#ifndef WITH_MOLTEN
        volkLoadInstance(instance);
#endif
    }

    void SkyboxViewer::createSurface() {
        if (!SDL_Vulkan_CreateSurface(window, instance, &surface))
            throw mx::Exception("Failed to create Vulkan surface!");
    }

    bool SkyboxViewer::isDeviceSuitable(VkPhysicalDevice dev) {
        QueueFamilyIndices indices = findQueueFamilies(dev);
        const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        uint32_t extCount;
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> avail(extCount);
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, avail.data());
        std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());
        for (const auto& e : avail) required.erase(e.extensionName);
        bool swapOk = false;
        if (required.empty()) {
            auto sc = querySwapChainSupport(dev);
            swapOk = !sc.formats.empty() && !sc.presentModes.empty();
        }
        return indices.isComplete() && required.empty() && swapOk;
    }

    void SkyboxViewer::pickPhysicalDevice() {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(instance, &count, nullptr);
        if (!count) throw mx::Exception("No Vulkan GPUs found!");
        std::vector<VkPhysicalDevice> devs(count);
        vkEnumeratePhysicalDevices(instance, &count, devs.data());
        for (auto &d : devs) if (isDeviceSuitable(d)) { physicalDevice = d; break; }
        if (physicalDevice == VK_NULL_HANDLE) throw mx::Exception("No suitable GPU!");
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physicalDevice, &props);
        std::cout << ">> GPU: " << props.deviceName << "\n";
    }

    void SkyboxViewer::createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        std::set<uint32_t> uniqueQueues = {indices.graphicsFamily.value(), indices.presentFamily.value()};
        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        float prio = 1.0f;
        for (uint32_t qf : uniqueQueues) {
            VkDeviceQueueCreateInfo qi{};
            qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qi.queueFamilyIndex = qf;
            qi.queueCount = 1;
            qi.pQueuePriorities = &prio;
            queueInfos.push_back(qi);
        }

        VkPhysicalDeviceFeatures features{};
        features.samplerAnisotropy = VK_TRUE;

        std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#ifdef WITH_MOLTEN
        uint32_t extCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> avail(extCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, avail.data());
        for (auto &e : avail) if (strcmp(e.extensionName, "VK_KHR_portability_subset") == 0) { deviceExtensions.push_back("VK_KHR_portability_subset"); break; }
#endif

        VkDeviceCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        ci.queueCreateInfoCount = (uint32_t)queueInfos.size();
        ci.pQueueCreateInfos = queueInfos.data();
        ci.pEnabledFeatures = &features;
        ci.enabledExtensionCount = (uint32_t)deviceExtensions.size();
        ci.ppEnabledExtensionNames = deviceExtensions.data();

        if (vkCreateDevice(physicalDevice, &ci, nullptr, &device) != VK_SUCCESS)
            throw mx::Exception("Failed to create logical device!");
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
#ifndef WITH_MOLTEN
        volkLoadDevice(device);
#endif
    }

    QueueFamilyIndices SkyboxViewer::findQueueFamilies(VkPhysicalDevice dev) {
        QueueFamilyIndices idx;
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, families.data());
        int i = 0;
        for (auto &qf : families) {
            if (qf.queueFlags & VK_QUEUE_GRAPHICS_BIT) idx.graphicsFamily = i;
            VkBool32 present = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &present);
            if (present) idx.presentFamily = i;
            if (idx.isComplete()) break;
            i++;
        }
        return idx;
    }

    VkSurfaceFormatKHR SkyboxViewer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
        for (auto &f : formats)
            if (f.format == VK_FORMAT_R8G8B8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return f;
        return formats[0];
    }

    VkPresentModeKHR SkyboxViewer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes) {
        for (auto &m : modes) if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D SkyboxViewer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps) {
        if (caps.currentExtent.width != UINT32_MAX) return caps.currentExtent;
        VkExtent2D ext = {(uint32_t)w, (uint32_t)h};
        ext.width = std::max(caps.minImageExtent.width, std::min(caps.maxImageExtent.width, ext.width));
        ext.height = std::max(caps.minImageExtent.height, std::min(caps.maxImageExtent.height, ext.height));
        return ext;
    }

    SwapChainSupportDetails SkyboxViewer::querySwapChainSupport(VkPhysicalDevice dev) {
        SwapChainSupportDetails d;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &d.capabilities);
        uint32_t fc = 0; vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &fc, nullptr);
        if (fc) { d.formats.resize(fc); vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &fc, d.formats.data()); }
        uint32_t pc = 0; vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &pc, nullptr);
        if (pc) { d.presentModes.resize(pc); vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &pc, d.presentModes.data()); }
        return d;
    }

    void SkyboxViewer::createSwapChain() {
        auto sc = querySwapChainSupport(physicalDevice);
        auto fmt = chooseSwapSurfaceFormat(sc.formats);
        auto mode = chooseSwapPresentMode(sc.presentModes);
        auto extent = chooseSwapExtent(sc.capabilities);
        uint32_t imgCount = sc.capabilities.minImageCount + 1;
        if (sc.capabilities.maxImageCount > 0 && imgCount > sc.capabilities.maxImageCount)
            imgCount = sc.capabilities.maxImageCount;

        VkSwapchainCreateInfoKHR ci{};
        ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.surface = surface;
        ci.minImageCount = imgCount;
        ci.imageFormat = fmt.format;
        ci.imageColorSpace = fmt.colorSpace;
        ci.imageExtent = extent;
        ci.imageArrayLayers = 1;
        ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        auto idx = findQueueFamilies(physicalDevice);
        uint32_t qfi[] = {idx.graphicsFamily.value(), idx.presentFamily.value()};
        if (idx.graphicsFamily != idx.presentFamily) {
            ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            ci.queueFamilyIndexCount = 2;
            ci.pQueueFamilyIndices = qfi;
        } else {
            ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        ci.preTransform = sc.capabilities.currentTransform;
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        ci.presentMode = mode;
        ci.clipped = VK_TRUE;
        ci.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &ci, nullptr, &swapChain) != VK_SUCCESS)
            throw mx::Exception("Failed to create swap chain!");
        vkGetSwapchainImagesKHR(device, swapChain, &imgCount, nullptr);
        swapChainImages.resize(imgCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imgCount, swapChainImages.data());
        swapChainImageFormat = fmt.format;
        swapChainExtent = extent;
    }

    void SkyboxViewer::createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.image = swapChainImages[i];
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.format = swapChainImageFormat;
            ci.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
            ci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            if (vkCreateImageView(device, &ci, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
                throw mx::Exception("Failed to create image views!");
        }
    }

    void SkyboxViewer::createRenderPass() {
        VkAttachmentDescription colorAtt{};
        colorAtt.format = swapChainImageFormat;
        colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depthAtt{};
        depthAtt.format = depthFormat;
        depthAtt.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAtt.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAtt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;

        VkSubpassDependency dep{};
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> atts = {colorAtt, depthAtt};
        VkRenderPassCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        ci.attachmentCount = (uint32_t)atts.size();
        ci.pAttachments = atts.data();
        ci.subpassCount = 1;
        ci.pSubpasses = &subpass;
        ci.dependencyCount = 1;
        ci.pDependencies = &dep;

        if (vkCreateRenderPass(device, &ci, nullptr, &renderPass) != VK_SUCCESS)
            throw mx::Exception("Failed to create render pass!");
    }

    VkFormat SkyboxViewer::findSupportedFormat(const std::vector<VkFormat>& cands, VkImageTiling tiling, VkFormatFeatureFlags feats) {
        for (auto f : cands) {
            VkFormatProperties p;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, f, &p);
            if (tiling == VK_IMAGE_TILING_LINEAR && (p.linearTilingFeatures & feats) == feats) return f;
            if (tiling == VK_IMAGE_TILING_OPTIMAL && (p.optimalTilingFeatures & feats) == feats) return f;
        }
        throw mx::Exception("Failed to find supported format!");
    }

    VkFormat SkyboxViewer::findDepthFormat() {
        return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    void SkyboxViewer::createDepthResources() {
        depthFormat = findDepthFormat();
        
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {swapChainExtent.width, swapChainExtent.height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        if (vkCreateImage(device, &imageInfo, nullptr, &depthImage) != VK_SUCCESS)
            throw mx::Exception("Failed to create depth image!");
        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(device, depthImage, &memReq);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkAllocateMemory(device, &allocInfo, nullptr, &depthImageMemory) != VK_SUCCESS)
            throw mx::Exception("Failed to allocate depth memory!");
        vkBindImageMemory(device, depthImage, depthImageMemory, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
        if (vkCreateImageView(device, &viewInfo, nullptr, &depthImageView) != VK_SUCCESS)
            throw mx::Exception("Failed to create depth image view!");
    }

    void SkyboxViewer::createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 0;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.descriptorCount = 1;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding = 1;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = 1;
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {samplerBinding, uboBinding};
        VkDescriptorSetLayoutCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ci.bindingCount = (uint32_t)bindings.size();
        ci.pBindings = bindings.data();
        if (vkCreateDescriptorSetLayout(device, &ci, nullptr, &descriptorSetLayout) != VK_SUCCESS)
            throw mx::Exception("Failed to create descriptor set layout!");
    }

    VkShaderModule SkyboxViewer::createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = code.size();
        ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule mod;
        if (vkCreateShaderModule(device, &ci, nullptr, &mod) != VK_SUCCESS)
            throw mx::Exception("Failed to create shader module!");
        return mod;
    }

    void SkyboxViewer::createGraphicsPipeline() {
        auto vertCode = mx::readFile(util.getFilePath("vert.spv"));
        auto fragCode = mx::readFile(util.getFilePath("frag.spv"));
        VkShaderModule vertMod = createShaderModule(vertCode);
        VkShaderModule fragMod = createShaderModule(fragCode);

        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertMod;
        vertStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragStage{};
        fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = fragMod;
        fragStage.pName = "main";

        VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(Vertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 3> attrs{};
        attrs[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)};
        attrs[1] = {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)};
        attrs[2] = {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)};

        VkPipelineVertexInputStateCreateInfo vertInput{};
        vertInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertInput.vertexBindingDescriptionCount = 1;
        vertInput.pVertexBindingDescriptions = &binding;
        vertInput.vertexAttributeDescriptionCount = (uint32_t)attrs.size();
        vertInput.pVertexAttributeDescriptions = attrs.data();

        VkPipelineInputAssemblyStateCreateInfo inputAsm{};
        inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAsm.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport vp{0, 0, (float)swapChainExtent.width, (float)swapChainExtent.height, 0, 1};
        VkRect2D scissor{{0, 0}, swapChainExtent};

        VkPipelineViewportStateCreateInfo vpState{};
        vpState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vpState.viewportCount = 1;
        vpState.pViewports = &vp;
        vpState.scissorCount = 1;
        vpState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rast{};
        rast.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rast.polygonMode = VK_POLYGON_MODE_FILL;
        rast.lineWidth = 1.0f;
        rast.cullMode = VK_CULL_MODE_NONE;
        rast.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo ms{};
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo ds{};
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.depthTestEnable = VK_TRUE;
        ds.depthWriteEnable = VK_FALSE; 
        ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        VkPipelineColorBlendAttachmentState blend{};
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo cb{};
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.attachmentCount = 1;
        cb.pAttachments = &blend;

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushRange.offset = 0;
        pushRange.size = sizeof(FractalPushConstants);

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &descriptorSetLayout;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;
        if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
            throw mx::Exception("Failed to create pipeline layout!");

        VkGraphicsPipelineCreateInfo pi{};
        pi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pi.stageCount = 2;
        pi.pStages = stages;
        pi.pVertexInputState = &vertInput;
        pi.pInputAssemblyState = &inputAsm;
        pi.pViewportState = &vpState;
        pi.pRasterizationState = &rast;
        pi.pMultisampleState = &ms;
        pi.pDepthStencilState = &ds;
        pi.pColorBlendState = &cb;
        pi.layout = pipelineLayout;
        pi.renderPass = renderPass;
        pi.subpass = 0;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pi, nullptr, &graphicsPipeline) != VK_SUCCESS)
            throw mx::Exception("Failed to create graphics pipeline!");

        vkDestroyShaderModule(device, fragMod, nullptr);
        vkDestroyShaderModule(device, vertMod, nullptr);
        std::cout << ">> Created skybox pipeline\n";
    }

    void SkyboxViewer::createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::array<VkImageView, 2> atts = {swapChainImageViews[i], depthImageView};
            VkFramebufferCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            ci.renderPass = renderPass;
            ci.attachmentCount = (uint32_t)atts.size();
            ci.pAttachments = atts.data();
            ci.width = swapChainExtent.width;
            ci.height = swapChainExtent.height;
            ci.layers = 1;
            if (vkCreateFramebuffer(device, &ci, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
                throw mx::Exception("Failed to create framebuffer!");
        }
    }

    void SkyboxViewer::createCommandPool() {
        auto idx = findQueueFamilies(physicalDevice);
        VkCommandPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        ci.queueFamilyIndex = idx.graphicsFamily.value();
        ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        if (vkCreateCommandPool(device, &ci, nullptr, &commandPool) != VK_SUCCESS)
            throw mx::Exception("Failed to create command pool!");
    }

    void SkyboxViewer::createCommandBuffers() {
        commandBuffers.resize(swapChainFramebuffers.size());
        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = commandPool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = (uint32_t)commandBuffers.size();
        if (vkAllocateCommandBuffers(device, &ai, commandBuffers.data()) != VK_SUCCESS)
            throw mx::Exception("Failed to allocate command buffers!");
    }

    void SkyboxViewer::createSyncObjects() {
        VkSemaphoreCreateInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vkCreateSemaphore(device, &si, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &si, nullptr, &renderFinishedSemaphore) != VK_SUCCESS)
            throw mx::Exception("Failed to create semaphores!");
    }

    void SkyboxViewer::createUniformBuffers() {
        VkDeviceSize size = sizeof(SkyboxUBO);
        size_t count = swapChainImages.size();
        uniformBuffers.resize(count);
        uniformBuffersMemory.resize(count);
        uniformBuffersMapped.resize(count);
        for (size_t i = 0; i < count; i++) {
            createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                uniformBuffers[i], uniformBuffersMemory[i]);
            vkMapMemory(device, uniformBuffersMemory[i], 0, size, 0, &uniformBuffersMapped[i]);
        }
    }

    void SkyboxViewer::createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 2> sizes{};
        sizes[0] = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, (uint32_t)swapChainImages.size()};
        sizes[1] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (uint32_t)swapChainImages.size()};

        VkDescriptorPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ci.poolSizeCount = (uint32_t)sizes.size();
        ci.pPoolSizes = sizes.data();
        ci.maxSets = (uint32_t)swapChainImages.size();
        if (vkCreateDescriptorPool(device, &ci, nullptr, &descriptorPool) != VK_SUCCESS)
            throw mx::Exception("Failed to create descriptor pool!");
    }

    void SkyboxViewer::createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
        VkDescriptorSetAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool = descriptorPool;
        ai.descriptorSetCount = (uint32_t)swapChainImages.size();
        ai.pSetLayouts = layouts.data();
        descriptorSets.resize(swapChainImages.size());
        if (vkAllocateDescriptorSets(device, &ai, descriptorSets.data()) != VK_SUCCESS)
            throw mx::Exception("Failed to allocate descriptor sets!");

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkDescriptorImageInfo imgInfo{};
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            
            
            if (useVideoTexture) {
                imgInfo.imageView = textureImageView;
                imgInfo.sampler = textureSampler;
            } else {
                imgInfo.imageView = cubemapImageView;
                imgInfo.sampler = cubemapSampler;
            }

            VkDescriptorBufferInfo bufInfo{};
            bufInfo.buffer = uniformBuffers[i];
            bufInfo.offset = 0;
            bufInfo.range = sizeof(SkyboxUBO);

            std::array<VkWriteDescriptorSet, 2> writes{};
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = descriptorSets[i];
            writes[0].dstBinding = 0;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[0].descriptorCount = 1;
            writes[0].pImageInfo = &imgInfo;

            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = descriptorSets[i];
            writes[1].dstBinding = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[1].descriptorCount = 1;
            writes[1].pBufferInfo = &bufInfo;

            vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
        }
    }

    

    void SkyboxViewer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkBuffer& buf, VkDeviceMemory& mem) {
        VkBufferCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        ci.size = size;
        ci.usage = usage;
        ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device, &ci, nullptr, &buf) != VK_SUCCESS)
            throw mx::Exception("Failed to create buffer!");
        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(device, buf, &req);
        VkMemoryAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = req.size;
        ai.memoryTypeIndex = findMemoryType(req.memoryTypeBits, props);
        if (vkAllocateMemory(device, &ai, nullptr, &mem) != VK_SUCCESS)
            throw mx::Exception("Failed to allocate buffer memory!");
        vkBindBufferMemory(device, buf, mem, 0);
    }

    uint32_t SkyboxViewer::findMemoryType(uint32_t filter, VkMemoryPropertyFlags props) {
        VkPhysicalDeviceMemoryProperties mp;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &mp);
        for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
            if ((filter & (1 << i)) && (mp.memoryTypes[i].propertyFlags & props) == props) return i;
        throw mx::Exception("Failed to find suitable memory type!");
    }

    void SkyboxViewer::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
        VkCommandBuffer cmd = beginSingleTimeCommands();
        VkBufferCopy region{};
        region.size = size;
        vkCmdCopyBuffer(cmd, src, dst, 1, &region);
        endSingleTimeCommands(cmd);
    }

    void SkyboxViewer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldL, VkImageLayout newL, uint32_t layerCount) {
        VkCommandBuffer cmd = beginSingleTimeCommands();
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldL;
        barrier.newLayout = newL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layerCount;

        VkPipelineStageFlags srcStage, dstStage;
        if (oldL == VK_IMAGE_LAYOUT_UNDEFINED && newL == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldL == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newL == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldL == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newL == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else {
            throw mx::Exception("Unsupported layout transition!");
        }
        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        endSingleTimeCommands(cmd);
    }

    void SkyboxViewer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t w, uint32_t h, uint32_t layer) {
        VkCommandBuffer cmd = beginSingleTimeCommands();
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = layer;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {w, h, 1};
        vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        endSingleTimeCommands(cmd);
    }

    VkCommandBuffer SkyboxViewer::beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandPool = commandPool;
        ai.commandBufferCount = 1;
        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &ai, &cmd);
        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &bi);
        return cmd;
    }

    void SkyboxViewer::endSingleTimeCommands(VkCommandBuffer cmd) {
        vkEndCommandBuffer(cmd);
        VkSubmitInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cmd;
        vkQueueSubmit(graphicsQueue, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
    }

    

    void SkyboxViewer::cleanupSwapChain() {
        for (auto fb : swapChainFramebuffers) if (fb) vkDestroyFramebuffer(device, fb, nullptr);
        swapChainFramebuffers.clear();
        if (!commandBuffers.empty()) {
            vkFreeCommandBuffers(device, commandPool, (uint32_t)commandBuffers.size(), commandBuffers.data());
            commandBuffers.clear();
        }
        if (graphicsPipeline) { vkDestroyPipeline(device, graphicsPipeline, nullptr); graphicsPipeline = VK_NULL_HANDLE; }
        if (pipelineLayout) { vkDestroyPipelineLayout(device, pipelineLayout, nullptr); pipelineLayout = VK_NULL_HANDLE; }
        if (renderPass) { vkDestroyRenderPass(device, renderPass, nullptr); renderPass = VK_NULL_HANDLE; }
        for (auto iv : swapChainImageViews) if (iv) vkDestroyImageView(device, iv, nullptr);
        swapChainImageViews.clear();
        if (swapChain) { vkDestroySwapchainKHR(device, swapChain, nullptr); swapChain = VK_NULL_HANDLE; }
        if (depthImageView) { vkDestroyImageView(device, depthImageView, nullptr); depthImageView = VK_NULL_HANDLE; }
        if (depthImage) { vkDestroyImage(device, depthImage, nullptr); depthImage = VK_NULL_HANDLE; }
        if (depthImageMemory) { vkFreeMemory(device, depthImageMemory, nullptr); depthImageMemory = VK_NULL_HANDLE; }

        for (size_t i = 0; i < uniformBuffers.size(); i++) {
            if (uniformBuffers[i]) vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            if (uniformBuffersMemory[i]) vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }
        uniformBuffers.clear();
        uniformBuffersMemory.clear();
        uniformBuffersMapped.clear();

        if (descriptorPool) { vkDestroyDescriptorPool(device, descriptorPool, nullptr); descriptorPool = VK_NULL_HANDLE; }
        descriptorSets.clear();
    }

    void SkyboxViewer::recreateSwapChain() {
        vkDeviceWaitIdle(device);
        cleanupSwapChain();
        createSwapChain();
        createImageViews();
        createDepthResources();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createDescriptorPool();
        createUniformBuffers();
        createDescriptorSets();
        createCommandBuffers();
    }

    void SkyboxViewer::cleanup() {
        vkDeviceWaitIdle(device);
        cleanupSwapChain();

        
        if (cubemapSampler) vkDestroySampler(device, cubemapSampler, nullptr);
        if (cubemapImageView) vkDestroyImageView(device, cubemapImageView, nullptr);
        if (cubemapImage) vkDestroyImage(device, cubemapImage, nullptr);
        if (cubemapImageMemory) vkFreeMemory(device, cubemapImageMemory, nullptr);

        
        if (textureSampler) vkDestroySampler(device, textureSampler, nullptr);
        if (textureImageView) vkDestroyImageView(device, textureImageView, nullptr);
        if (textureImage) vkDestroyImage(device, textureImage, nullptr);
        if (textureImageMemory) vkFreeMemory(device, textureImageMemory, nullptr);

        
        for (size_t i = 0; i < extSpriteBuffers.size(); i++) {
            if (extSpriteBuffers[i] != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, extSpriteBuffers[i], nullptr);
                vkFreeMemory(device, extSpriteMemory[i], nullptr);
            }
        }
        extSpriteBuffers.clear();
        extSpriteMemory.clear();
        extSpriteMapped.clear();
        
        if (extDescPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, extDescPool, nullptr);
            extDescPool = VK_NULL_HANDLE;
        }
        if (extPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, extPipelineLayout, nullptr);
            extPipelineLayout = VK_NULL_HANDLE;
        }
        if (extFragDescSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, extFragDescSetLayout, nullptr);
            extFragDescSetLayout = VK_NULL_HANDLE;
        }
        if (extVertDescSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, extVertDescSetLayout, nullptr);
            extVertDescSetLayout = VK_NULL_HANDLE;
        }

        
        if (cap.isOpened()) {
            cap.release();
        }

        model.cleanup(device);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        if (commandPool) vkDestroyCommandPool(device, commandPool, nullptr);
        if (device) vkDestroyDevice(device, nullptr);
        if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
        if (instance) vkDestroyInstance(instance, nullptr);
#ifndef WITH_MOLTEN
        volkFinalize();
#endif
        controller.close();
        if (window) { SDL_DestroyWindow(window); SDL_Quit(); }
    }

}
