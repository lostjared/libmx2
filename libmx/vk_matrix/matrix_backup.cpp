#include "vk.hpp"
#include "SDL.h"
#include <format>
#include <random>
#include <cmath>
#include <deque>
#include <unordered_map>

#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
#include "argz.hpp"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float generateRandomFloat(float min, float max) {
    static std::random_device rd;
    static std::default_random_engine eng(rd());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(eng);
}

// Simple vertex for skybox cube
struct SkyboxVertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
};

class Matrix3DWindow : public mx::VKWindow {
public:
    Matrix3DWindow(const std::string& path, int wx, int wy, bool full) 
        : mx::VKWindow("-[ Vulkan Matrix 3D ]-", wx, wy, full) {
        setPath(path);
        lastFrameTime = SDL_GetPerformanceCounter();
        
        // Camera setup - start inside the cube
        cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        cameraYaw = 0.0f;
        cameraPitch = 0.0f;
        insideMatrix = true;
        
        // Japanese character ranges
        codepointRanges = {
            {0x3041, 0x3096},  // Hiragana
            {0x30A0, 0x30FF},  // Katakana
        };
    }
    
    virtual ~Matrix3DWindow() {}
    
    void cleanup() override {
        cleanupMatrix();
        mx::VKWindow::cleanup();
    }
    
    void initVulkan() override {
        mx::VKWindow::initVulkan();
        initMatrix();
        initialized = true;
    }
    
    void proc() override {
        if (!initialized) {
            mx::VKWindow::proc();
            return;
        }
        
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float deltaTime = (currentTime - lastFrameTime) / (double)SDL_GetPerformanceFrequency();
        lastFrameTime = currentTime;
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        // Handle keyboard input for looking around
        const Uint8* keyState = SDL_GetKeyboardState(nullptr);
        const float rotateSpeed = 100.0f * deltaTime;
        
        if (insideMatrix) {
            if (keyState[SDL_SCANCODE_LEFT] || keyState[SDL_SCANCODE_A]) {
                cameraYaw -= rotateSpeed;
            }
            if (keyState[SDL_SCANCODE_RIGHT] || keyState[SDL_SCANCODE_D]) {
                cameraYaw += rotateSpeed;
            }
            if (keyState[SDL_SCANCODE_UP] || keyState[SDL_SCANCODE_W]) {
                cameraPitch += rotateSpeed;
            }
            if (keyState[SDL_SCANCODE_DOWN] || keyState[SDL_SCANCODE_S]) {
                cameraPitch -= rotateSpeed;
            }
            
            // Clamp pitch
            if (cameraPitch > 89.0f) cameraPitch = 89.0f;
            if (cameraPitch < -89.0f) cameraPitch = -89.0f;
            // Wrap yaw
            if (cameraYaw < 0.0f) cameraYaw += 360.0f;
            if (cameraYaw > 360.0f) cameraYaw -= 360.0f;
        }
        
        // Store delta time for draw() to use
        currentDeltaTime = deltaTime;
        
        mx::VKWindow::proc();
    }
    
    void event(SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    quit();
                    break;
                case SDLK_RETURN:
                    insideMatrix = !insideMatrix;
                    if (insideMatrix) {
                        cameraPitch = 0.0f;
                        cameraYaw = 0.0f;
                    }
                    break;
            }
        }
    }

private:
    Uint64 lastFrameTime;
    bool initialized = false;
    float currentDeltaTime = 0.0f;
    
    // Camera
    glm::vec3 cameraPosition;
    float cameraYaw;
    float cameraPitch;
    bool insideMatrix;
    float cubeRotationX = 0.0f;
    float cubeRotationY = 0.0f;
    
    // Matrix rain parameters
    static constexpr int TEXTURE_WIDTH = 1024;
    static constexpr int TEXTURE_HEIGHT = 1024;
    int charWidth = 24;
    int charHeight = 32;
    int numColumns;
    int numRows;
    
    std::vector<float> fallPositions;
    std::vector<float> fallSpeeds;
    std::vector<int> trailLengths;
    std::vector<float> columnBrightness;
    std::vector<bool> isHighlightColumn;
    std::vector<std::pair<int, int>> codepointRanges;
    
    // Vulkan resources
    VkBuffer skyboxVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory skyboxVertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer skyboxIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory skyboxIndexBufferMemory = VK_NULL_HANDLE;
    
    VkImage matrixTextureImage = VK_NULL_HANDLE;
    VkDeviceMemory matrixTextureMemory = VK_NULL_HANDLE;
    VkImageView matrixTextureView = VK_NULL_HANDLE;
    VkSampler matrixTextureSampler = VK_NULL_HANDLE;
    
    VkDescriptorSetLayout matrixDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool matrixDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet matrixDescriptorSet = VK_NULL_HANDLE;
    
    VkPipelineLayout matrixPipelineLayout = VK_NULL_HANDLE;
    VkPipeline matrixPipeline = VK_NULL_HANDLE;
    
    // Font for character rendering
    TTF_Font* matrixFont = nullptr;
    std::vector<uint32_t> texturePixels;
    
    uint32_t indexCount = 0;
    
    void initMatrix() {
        // Load font - keifont supports Japanese characters
        std::string fontPath = util.getFilePath("data/keifont.ttf");
        matrixFont = TTF_OpenFont(fontPath.c_str(), 28);
        if (!matrixFont) {
            throw mx::Exception("Failed to load matrix font: " + fontPath);
        }
        
        // Initialize rain parameters
        numColumns = TEXTURE_WIDTH / charWidth;
        numRows = TEXTURE_HEIGHT / charHeight;
        
        fallPositions.resize(numColumns);
        fallSpeeds.resize(numColumns);
        trailLengths.resize(numColumns);
        columnBrightness.resize(numColumns);
        isHighlightColumn.resize(numColumns);
        
        for (int i = 0; i < numColumns; ++i) {
            fallPositions[i] = static_cast<float>(rand() % numRows);
            fallSpeeds[i] = generateRandomFloat(3.0f, 10.0f);
            trailLengths[i] = rand() % 20 + 8;
            isHighlightColumn[i] = (rand() % 15 == 0);
            columnBrightness[i] = generateRandomFloat(0.7f, 1.3f);
        }
        
        texturePixels.resize(TEXTURE_WIDTH * TEXTURE_HEIGHT);
        
        // Create Vulkan resources
        createSkyboxGeometry();
        createMatrixTexture();
        createDescriptorSetLayout();
        createMatrixPipeline();
        createDescriptorPool();
        createDescriptorSet();
    }
    
    void createSkyboxGeometry() {
        // Create an inverted cube (normals face inward) for skybox
        float size = 10.0f;
        
        // Vertices for a cube with texture coords (Y flipped so rain falls down)
        std::vector<SkyboxVertex> vertices = {
            // Front face (looking from inside)
            {{-size, -size, -size}, {0.0f, 1.0f}},
            {{ size, -size, -size}, {1.0f, 1.0f}},
            {{ size,  size, -size}, {1.0f, 0.0f}},
            {{-size,  size, -size}, {0.0f, 0.0f}},
            // Back face
            {{ size, -size,  size}, {0.0f, 1.0f}},
            {{-size, -size,  size}, {1.0f, 1.0f}},
            {{-size,  size,  size}, {1.0f, 0.0f}},
            {{ size,  size,  size}, {0.0f, 0.0f}},
            // Left face
            {{-size, -size,  size}, {0.0f, 1.0f}},
            {{-size, -size, -size}, {1.0f, 1.0f}},
            {{-size,  size, -size}, {1.0f, 0.0f}},
            {{-size,  size,  size}, {0.0f, 0.0f}},
            // Right face
            {{ size, -size, -size}, {0.0f, 1.0f}},
            {{ size, -size,  size}, {1.0f, 1.0f}},
            {{ size,  size,  size}, {1.0f, 0.0f}},
            {{ size,  size, -size}, {0.0f, 0.0f}},
            // Top face
            {{-size,  size, -size}, {0.0f, 0.0f}},
            {{ size,  size, -size}, {1.0f, 0.0f}},
            {{ size,  size,  size}, {1.0f, 1.0f}},
            {{-size,  size,  size}, {0.0f, 1.0f}},
            // Bottom face
            {{-size, -size,  size}, {0.0f, 0.0f}},
            {{ size, -size,  size}, {1.0f, 0.0f}},
            {{ size, -size, -size}, {1.0f, 1.0f}},
            {{-size, -size, -size}, {0.0f, 1.0f}},
        };
        
        // Indices for inverted cube (counter-clockwise from inside)
        std::vector<uint32_t> indices = {
            0, 1, 2, 2, 3, 0,       // Front
            4, 5, 6, 6, 7, 4,       // Back
            8, 9, 10, 10, 11, 8,    // Left
            12, 13, 14, 14, 15, 12, // Right
            16, 17, 18, 18, 19, 16, // Top
            20, 21, 22, 22, 23, 20, // Bottom
        };
        
        indexCount = static_cast<uint32_t>(indices.size());
        
        // Create vertex buffer
        VkDeviceSize vertexBufferSize = vertices.size() * sizeof(SkyboxVertex);
        createBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     skyboxVertexBuffer, skyboxVertexBufferMemory);
        
        void* data;
        vkMapMemory(device, skyboxVertexBufferMemory, 0, vertexBufferSize, 0, &data);
        memcpy(data, vertices.data(), vertexBufferSize);
        vkUnmapMemory(device, skyboxVertexBufferMemory);
        
        // Create index buffer
        VkDeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);
        createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     skyboxIndexBuffer, skyboxIndexBufferMemory);
        
        vkMapMemory(device, skyboxIndexBufferMemory, 0, indexBufferSize, 0, &data);
        memcpy(data, indices.data(), indexBufferSize);
        vkUnmapMemory(device, skyboxIndexBufferMemory);
    }
    
    void createMatrixTexture() {
        // Create texture image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = TEXTURE_WIDTH;
        imageInfo.extent.height = TEXTURE_HEIGHT;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_LINEAR;  // Linear for CPU access
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        if (vkCreateImage(device, &imageInfo, nullptr, &matrixTextureImage) != VK_SUCCESS) {
            throw mx::Exception("Failed to create matrix texture image!");
        }
        
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, matrixTextureImage, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        
        if (vkAllocateMemory(device, &allocInfo, nullptr, &matrixTextureMemory) != VK_SUCCESS) {
            throw mx::Exception("Failed to allocate matrix texture memory!");
        }
        
        vkBindImageMemory(device, matrixTextureImage, matrixTextureMemory, 0);
        
        // Transition to shader read layout
        transitionImageLayout(matrixTextureImage, VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        // Create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = matrixTextureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(device, &viewInfo, nullptr, &matrixTextureView) != VK_SUCCESS) {
            throw mx::Exception("Failed to create matrix texture view!");
        }
        
        // Create sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        
        if (vkCreateSampler(device, &samplerInfo, nullptr, &matrixTextureSampler) != VK_SUCCESS) {
            throw mx::Exception("Failed to create matrix texture sampler!");
        }
    }
    
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
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
        
        if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_HOST_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            destinationStage = VK_PIPELINE_STAGE_HOST_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_HOST_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        
        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0,
                             0, nullptr, 0, nullptr, 1, &barrier);
        
        endSingleTimeCommands(commandBuffer);
    }
    
    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }
    
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);
        
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
    
    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 0;
        samplerBinding.descriptorCount = 1;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.pImmutableSamplers = nullptr;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        
        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding = 1;
        uboBinding.descriptorCount = 1;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.pImmutableSamplers = nullptr;
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        
        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {samplerBinding, uboBinding};
        
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        
        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &matrixDescriptorSetLayout) != VK_SUCCESS) {
            throw mx::Exception("Failed to create descriptor set layout!");
        }
    }
    
    void createMatrixPipeline() {
        // Load shaders
        auto vertShaderCode = mx::readFile(util.getFilePath("data/matrix_vert.spv"));
        auto fragShaderCode = mx::readFile(util.getFilePath("data/matrix_frag.spv"));
        
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
        
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";
        
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";
        
        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
        
        // Vertex input
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(SkyboxVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(SkyboxVertex, pos);
        
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(SkyboxVertex, texCoord);
        
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
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
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
        rasterizer.cullMode = VK_CULL_MODE_NONE;  // No culling for skybox
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
        
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        
        // Pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &matrixDescriptorSetLayout;
        
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &matrixPipelineLayout) != VK_SUCCESS) {
            throw mx::Exception("Failed to create pipeline layout!");
        }
        
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
        pipelineInfo.layout = matrixPipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &matrixPipeline) != VK_SUCCESS) {
            throw mx::Exception("Failed to create graphics pipeline!");
        }
        
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }
    
    void createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = 1;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[1].descriptorCount = 1;
        
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1;
        
        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &matrixDescriptorPool) != VK_SUCCESS) {
            throw mx::Exception("Failed to create descriptor pool!");
        }
    }
    
    void createDescriptorSet() {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = matrixDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &matrixDescriptorSetLayout;
        
        if (vkAllocateDescriptorSets(device, &allocInfo, &matrixDescriptorSet) != VK_SUCCESS) {
            throw mx::Exception("Failed to allocate descriptor set!");
        }
        
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = matrixTextureView;
        imageInfo.sampler = matrixTextureSampler;
        
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[0];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(mx::UniformBufferObject);
        
        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = matrixDescriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &imageInfo;
        
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = matrixDescriptorSet;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &bufferInfo;
        
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
    
    std::string unicodeToUTF8(int codepoint) {
        std::string utf8;
        if (codepoint <= 0x7F) {
            utf8 += static_cast<char>(codepoint);
        } else if (codepoint <= 0x7FF) {
            utf8 += static_cast<char>((codepoint >> 6) | 0xC0);
            utf8 += static_cast<char>((codepoint & 0x3F) | 0x80);
        } else if (codepoint <= 0xFFFF) {
            utf8 += static_cast<char>((codepoint >> 12) | 0xE0);
            utf8 += static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80);
            utf8 += static_cast<char>((codepoint & 0x3F) | 0x80);
        }
        return utf8;
    }
    
    int getRandomCodepoint() {
        int rangeIndex = rand() % codepointRanges.size();
        int start = codepointRanges[rangeIndex].first;
        int end = codepointRanges[rangeIndex].second;
        return start + rand() % (end - start + 1);
    }
    
    void updateMatrixRain(float deltaTime) {
        // Clear texture to black
        memset(texturePixels.data(), 0, texturePixels.size() * sizeof(uint32_t));
        
        // Update and render each column
        for (int col = 0; col < numColumns; ++col) {
            // Occasionally change speed
            if (rand() % 100 == 0) {
                fallSpeeds[col] = generateRandomFloat(3.0f, 10.0f);
            }
            
            fallPositions[col] += fallSpeeds[col] * deltaTime;
            
            // Wrap around
            if (fallPositions[col] >= numRows) {
                fallPositions[col] -= numRows;
                trailLengths[col] = rand() % 20 + 8;
                isHighlightColumn[col] = (rand() % 15 == 0);
                columnBrightness[col] = generateRandomFloat(0.7f, 1.3f);
            }
            
            // Render trail
            for (int i = 0; i < trailLengths[col]; ++i) {
                int row = static_cast<int>(fallPositions[col] - i + numRows) % numRows;
                
                // Get character
                int codepoint = getRandomCodepoint();
                std::string charStr = unicodeToUTF8(codepoint);
                
                // Calculate color based on position in trail
                SDL_Color color;
                if (i == 0) {
                    // Head - bright white/green
                    if (isHighlightColumn[col]) {
                        color = {255, 255, 255, 255};
                    } else {
                        color = {180, 255, 180, 255};
                    }
                } else {
                    // Trail - fading green
                    float intensity = 1.0f - (float)i / (float)trailLengths[col];
                    intensity = powf(intensity, 1.5f) * columnBrightness[col];
                    if (intensity < 0.0f) intensity = 0.0f;
                    if (intensity > 1.0f) intensity = 1.0f;
                    
                    uint8_t green = static_cast<uint8_t>(255 * intensity);
                    uint8_t red = static_cast<uint8_t>(50 * intensity);
                    uint8_t alpha = static_cast<uint8_t>(255 * intensity);
                    if (alpha < 30) alpha = 30;
                    
                    color = {red, green, 0, alpha};
                }
                
                // Render character to texture
                renderCharToTexture(charStr, col * charWidth, row * charHeight, color);
            }
        }
        
        // Update GPU texture
        updateGPUTexture();
    }
    
    void renderCharToTexture(const std::string& charStr, int x, int y, SDL_Color color) {
        if (!matrixFont) return;
        
        SDL_Surface* charSurface = TTF_RenderUTF8_Blended(matrixFont, charStr.c_str(), color);
        if (!charSurface) {
            // Fallback to a simple character if Japanese fails
            charSurface = TTF_RenderUTF8_Blended(matrixFont, "#", color);
            if (!charSurface) return;
        }
        
        // Copy pixels to texture
        if (SDL_MUSTLOCK(charSurface)) {
            SDL_LockSurface(charSurface);
        }
        uint32_t* srcPixels = (uint32_t*)charSurface->pixels;
        
        for (int py = 0; py < charSurface->h && (y + py) < TEXTURE_HEIGHT; ++py) {
            for (int px = 0; px < charSurface->w && (x + px) < TEXTURE_WIDTH; ++px) {
                if (x + px >= 0 && y + py >= 0) {
                    int srcIdx = py * charSurface->w + px;
                    int dstIdx = (y + py) * TEXTURE_WIDTH + (x + px);
                    
                    uint32_t srcPixel = srcPixels[srcIdx];
                    uint8_t srcAlpha = (srcPixel >> 24) & 0xFF;
                    
                    if (srcAlpha > 0) {
                        texturePixels[dstIdx] = srcPixel;
                    }
                }
            }
        }
        
        if (SDL_MUSTLOCK(charSurface)) {
            SDL_UnlockSurface(charSurface);
        }
        SDL_FreeSurface(charSurface);
    }
    
    void updateGPUTexture() {
        // Transition to GENERAL for host write
        transitionImageLayout(matrixTextureImage, VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

        // Map texture memory and copy pixels
        void* data;
        vkMapMemory(device, matrixTextureMemory, 0, texturePixels.size() * sizeof(uint32_t), 0, &data);
        memcpy(data, texturePixels.data(), texturePixels.size() * sizeof(uint32_t));
        vkUnmapMemory(device, matrixTextureMemory);

        // Transition back to shader read layout
        transitionImageLayout(matrixTextureImage, VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    
    void draw() override {
        // Safety checks for null Vulkan handles with debug output
        if (device == VK_NULL_HANDLE || swapChain == VK_NULL_HANDLE) {
            std::cerr << "Device or swapChain is null" << std::endl;
            return;
        }
        if (matrixPipeline == VK_NULL_HANDLE || matrixPipelineLayout == VK_NULL_HANDLE) {
            std::cerr << "matrixPipeline=" << matrixPipeline << " matrixPipelineLayout=" << matrixPipelineLayout << std::endl;
            return;
        }
        if (skyboxVertexBuffer == VK_NULL_HANDLE || skyboxIndexBuffer == VK_NULL_HANDLE) {
            std::cerr << "skyboxVertexBuffer=" << skyboxVertexBuffer << " skyboxIndexBuffer=" << skyboxIndexBuffer << std::endl;
            return;
        }
        
        // Wait for previous frame to complete
        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFence);
        
        // Update matrix rain after fence wait (safe to modify texture now)
        updateMatrixRain(currentDeltaTime);
        
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                                 imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw mx::Exception("Failed to acquire swap chain image!");
        }
        
        // Update uniform buffer
        mx::UniformBufferObject ubo{};
        ubo.model = glm::mat4(1.0f);
        
        if (insideMatrix) {
            // Inside - look around with yaw/pitch
            glm::vec3 direction;
            direction.x = cos(glm::radians(cameraPitch)) * cos(glm::radians(cameraYaw));
            direction.y = sin(glm::radians(cameraPitch));
            direction.z = cos(glm::radians(cameraPitch)) * sin(glm::radians(cameraYaw));
            
            ubo.view = glm::lookAt(glm::vec3(0.0f), glm::normalize(direction), glm::vec3(0.0f, 1.0f, 0.0f));
        } else {
            // Outside - rotating cube
            cubeRotationX += 0.5f;
            cubeRotationY += 0.3f;
            
            ubo.model = glm::rotate(ubo.model, glm::radians(cubeRotationY), glm::vec3(0.0f, 1.0f, 0.0f));
            ubo.model = glm::rotate(ubo.model, glm::radians(cubeRotationX), glm::vec3(1.0f, 0.0f, 0.0f));
            
            ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 25.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        
        ubo.proj = glm::perspective(glm::radians(insideMatrix ? 90.0f : 45.0f),
                                     (float)swapChainExtent.width / (float)swapChainExtent.height,
                                     0.1f, 100.0f);
        ubo.proj[1][1] *= -1;
        
        // Safely update uniform buffer
        if (uniformBuffersMapped.size() > imageIndex && uniformBuffersMapped[imageIndex] != nullptr) {
            memcpy(uniformBuffersMapped[imageIndex], &ubo, sizeof(ubo));
        }
        
        // Record command buffer
        if (commandBuffers.size() <= imageIndex) {
            return;
        }
        
        vkResetCommandBuffer(commandBuffers[imageIndex], 0);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
            throw mx::Exception("Failed to begin recording command buffer!");
        }
        
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;
        
        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        
        vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, matrixPipeline);
        
        VkBuffer vertexBuffers[] = {skyboxVertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffers[imageIndex], skyboxIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                matrixPipelineLayout, 0, 1, &matrixDescriptorSet, 0, nullptr);
        
        vkCmdDrawIndexed(commandBuffers[imageIndex], indexCount, 1, 0, 0, 0);
        
        vkCmdEndRenderPass(commandBuffers[imageIndex]);
        
        if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
            throw mx::Exception("Failed to record command buffer!");
        }
        
        // Submit
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
            throw mx::Exception("Failed to submit draw command buffer!");
        }
        
        // Present
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        
        vkQueuePresentKHR(presentQueue, &presentInfo);
        
        // Wait for present to complete to avoid flashing with multiple swapchain images
        vkQueueWaitIdle(presentQueue);
    }
    
    void cleanupMatrix() {
        vkDeviceWaitIdle(device);
        
        if (matrixFont) {
            TTF_CloseFont(matrixFont);
            matrixFont = nullptr;
        }
        
        if (matrixPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, matrixPipeline, nullptr);
        }
        if (matrixPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, matrixPipelineLayout, nullptr);
        }
        if (matrixDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, matrixDescriptorPool, nullptr);
        }
        if (matrixDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, matrixDescriptorSetLayout, nullptr);
        }
        if (matrixTextureSampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, matrixTextureSampler, nullptr);
        }
        if (matrixTextureView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, matrixTextureView, nullptr);
        }
        if (matrixTextureImage != VK_NULL_HANDLE) {
            vkDestroyImage(device, matrixTextureImage, nullptr);
        }
        if (matrixTextureMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, matrixTextureMemory, nullptr);
        }
        if (skyboxVertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, skyboxVertexBuffer, nullptr);
        }
        if (skyboxVertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, skyboxVertexBufferMemory, nullptr);
        }
        if (skyboxIndexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, skyboxIndexBuffer, nullptr);
        }
        if (skyboxIndexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, skyboxIndexBufferMemory, nullptr);
        }
    }
};

#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    
    try {
        Matrix3DWindow window(args.path, args.width, args.height, false);
        window.initVulkan();  // Initialize Vulkan before starting the loop
        window.loop();
    } catch (const mx::Exception &e) {
        std::cerr << "Exception: " << e.text() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

#endif
