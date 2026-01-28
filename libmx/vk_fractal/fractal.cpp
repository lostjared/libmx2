#include "vk.hpp"
#include "SDL.h"
#include "loadpng.hpp"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
#include "argz.hpp"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct FractalPushConstants {
    float centerX;
    float centerY;
    float zoom;
    int maxIterations;
    float time;
};

class FractalWindow : public mx::VKWindow {

public:
    
    VkPipeline fractalPipeline = VK_NULL_HANDLE;
    VkPipelineLayout fractalPipelineLayout = VK_NULL_HANDLE;
    
    
    double centerX = -0.5;
    double centerY = 0.0;
    double zoom = 1.0;
    int maxIterations = 256;
    float animTime = 0.0f;
    
    
    bool mouseDragging = false;
    int lastMouseX = 0, lastMouseY = 0;
    double dragStartCenterX = 0.0;
    double dragStartCenterY = 0.0;
    
    int screenshotCounter = 0;
    bool captureNextFrame = false;
    
    FractalWindow(const std::string& path, int wx, int wy, bool full) 
        : mx::VKWindow("-[ Mandelbrot Fractal ]-", wx, wy, full) {
        setPath(path);
    }
    
    virtual ~FractalWindow() {}
    
    void cleanup() override {
        if (device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device);
            
            if (fractalPipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(device, fractalPipeline, nullptr);
                fractalPipeline = VK_NULL_HANDLE;
            }
            if (fractalPipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(device, fractalPipelineLayout, nullptr);
                fractalPipelineLayout = VK_NULL_HANDLE;
            }
        }
        mx::VKWindow::cleanup();
    }
    
    void initVulkan() override {
        mx::VKWindow::initVulkan();
        createFullscreenQuad();
        createFractalPipeline();
    }
    
    void saveScreenshot(uint32_t imageIndex) {
        vkDeviceWaitIdle(device);
        
        uint32_t width = swapChainExtent.width;
        uint32_t height = swapChainExtent.height;
        
        VkDeviceSize bufferSize = width * height * 4;
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);
        
        
        VkImage srcImage = swapChainImages[imageIndex];
        VkCommandBuffer cmdBuffer = beginSingleTimeCommands();
        
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = srcImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        
        vkCmdPipelineBarrier(cmdBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
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
        
        vkCmdCopyImageToBuffer(cmdBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            stagingBuffer, 1, &region);
        
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        
        vkCmdPipelineBarrier(cmdBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        endSingleTimeCommands(cmdBuffer);
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        std::vector<uint8_t> rgbaData(width * height * 4);
        uint8_t* src = static_cast<uint8_t*>(data);
        for (uint32_t i = 0; i < width * height; i++) {
            rgbaData[i * 4 + 0] = src[i * 4 + 2]; 
            rgbaData[i * 4 + 1] = src[i * 4 + 1]; 
            rgbaData[i * 4 + 2] = src[i * 4 + 0]; 
            rgbaData[i * 4 + 3] = 255;            
        }
        
        vkUnmapMemory(device, stagingBufferMemory);
        std::ostringstream filename;
        filename << "mandelbrot_" << std::setfill('0') << std::setw(4) << screenshotCounter++ << ".png";
        
        if (png::SavePNG_RGBA(filename.str().c_str(), rgbaData.data(), width, height)) {
            std::cout << "Screenshot saved: " << filename.str() << std::endl;
        } else {
            std::cerr << "Failed to save screenshot: " << filename.str() << std::endl;
        }
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void recreateSwapChain() {
        vkDeviceWaitIdle(device);

        if (fractalPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, fractalPipeline, nullptr);
            fractalPipeline = VK_NULL_HANDLE;
        }
        if (fractalPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, fractalPipelineLayout, nullptr);
            fractalPipelineLayout = VK_NULL_HANDLE;
        }

        mx::VKWindow::recreateSwapChain();

        createFullscreenQuad();
        createFractalPipeline();
    }
    
    void createFullscreenQuad() {
        std::vector<mx::Vertex> vertices = {
            {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}
        };
        
        std::vector<uint32_t> indices = {
            0, 1, 2,
            2, 3, 0
        };
        
        indexCount = static_cast<uint32_t>(indices.size());
        
        VkDeviceSize vertexBufferSize = sizeof(mx::Vertex) * vertices.size();
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);
        
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, vertexBufferSize, 0, &data);
        memcpy(data, vertices.data(), vertexBufferSize);
        vkUnmapMemory(device, stagingBufferMemory);
        
        createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
        
        copyBuffer(stagingBuffer, vertexBuffer, vertexBufferSize);
        
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
        
        VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
        
        createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);
        
        vkMapMemory(device, stagingBufferMemory, 0, indexBufferSize, 0, &data);
        memcpy(data, indices.data(), indexBufferSize);
        vkUnmapMemory(device, stagingBufferMemory);
        
        createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
        
        copyBuffer(stagingBuffer, indexBuffer, indexBufferSize);
        
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
    
    void createFractalPipeline() {
        
        auto vertShaderCode = mx::readFile(util.getFilePath("mandelbrot_vert.spv"));
        auto fragShaderCode = mx::readFile(util.getFilePath("mandelbrot_frag.spv"));
        
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
        
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
        
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(mx::Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(mx::Vertex, pos);
        
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(mx::Vertex, texCoord);
        
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(mx::Vertex, normal);
        
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
        
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = nullptr;
        viewportState.scissorCount = 1;
        viewportState.pScissors = nullptr;

        std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
        
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
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
        
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(FractalPushConstants);
        
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &fractalPipelineLayout) != VK_SUCCESS) {
            throw mx::Exception("Failed to create fractal pipeline layout!");
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
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = fractalPipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &fractalPipeline) != VK_SUCCESS) {
            throw mx::Exception("Failed to create fractal graphics pipeline!");
        }
        
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        
        std::cout << ">> [FractalPipeline] Created Mandelbrot fractal pipeline\n";
    }
    
    virtual void event(SDL_Event& e) override {
        if (e.type == SDL_QUIT) {
            quit();
            return;
        }
        
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    quit();
                    break;
                case SDLK_r:
                    centerX = -0.5;
                    centerY = 0.0;
                    zoom = 1.0;
                    maxIterations = 256;
                    break;
                case SDLK_PLUS:
                case SDLK_EQUALS:
                    maxIterations = std::min(maxIterations + 50, 2000);
                    break;
                case SDLK_MINUS:
                    maxIterations = std::max(maxIterations - 50, 50);
                    break;
                case SDLK_1:
                    centerX = -0.745;
                    centerY = 0.113;
                    zoom = 50.0;
                    break;
                case SDLK_2:
                    centerX = 0.275;
                    centerY = 0.0;
                    zoom = 10.0;
                    break;
                case SDLK_3:
                    centerX = -0.761574;
                    centerY = -0.0847596;
                    zoom = 200.0;
                    break;
                case SDLK_p:
                case SDLK_F12:
                    captureNextFrame = true;
                    break;
            }
        }
        
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (e.button.button == SDL_BUTTON_LEFT) {
                mouseDragging = true;
                lastMouseX = e.button.x;
                lastMouseY = e.button.y;
                dragStartCenterX = centerX;
                dragStartCenterY = centerY;
            }
        }
        
        if (e.type == SDL_MOUSEBUTTONUP) {
            if (e.button.button == SDL_BUTTON_LEFT) {
                mouseDragging = false;
            }
        }
        
        if (e.type == SDL_MOUSEMOTION) {
            if (mouseDragging) {
                int deltaX = e.motion.x - lastMouseX;
                int deltaY = e.motion.y - lastMouseY;
                
                double scale = 2.0 / (zoom * std::min(w, h));
                centerX = dragStartCenterX - deltaX * scale;
                centerY = dragStartCenterY + deltaY * scale;
            }
        }
        
        if (e.type == SDL_MOUSEWHEEL) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            double scale = 2.0 / (zoom * std::min(w, h));
            double mouseRealBefore = (mouseX - w / 2.0) * scale + centerX;
            double mouseImagBefore = (h / 2.0 - mouseY) * scale + centerY;
            double zoomFactor = (e.wheel.y > 0) ? 1.2 : (1.0 / 1.2);
            zoom *= zoomFactor;
            if (zoom < 0.5) zoom = 0.5;
            if (zoom > 1e14) zoom = 1e14;
            scale = 2.0 / (zoom * std::min(w, h));
            double mouseRealAfter = (mouseX - w / 2.0) * scale + centerX;
            double mouseImagAfter = (h / 2.0 - mouseY) * scale + centerY;
            centerX += mouseRealBefore - mouseRealAfter;
            centerY += mouseImagBefore - mouseImagAfter;
            if (e.wheel.y > 0 && zoom > 10) {
                maxIterations = std::min(maxIterations + 10, 2000);
            }
        }
    
        if (e.type == SDL_FINGERMOTION) {
            float deltaX = e.tfinger.dx * w;
            float deltaY = e.tfinger.dy * h;
            
            double scale = 2.0 / (zoom * std::min(w, h));
            centerX -= deltaX * scale;
            centerY += deltaY * scale;
        }
    }
    
    virtual void proc() override {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        static Uint64 lastFrameTime = currentTime;
        float deltaTime = (currentTime - lastFrameTime) / (double)SDL_GetPerformanceFrequency();
        lastFrameTime = currentTime;
        
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        animTime += deltaTime;
        
        const Uint8* keyState = SDL_GetKeyboardState(nullptr);
        double moveSpeed = 0.3 * deltaTime / zoom;  
        const double minX = -2.5, maxX = 1.5;
        const double minY = -1.5, maxY = 1.5;
        
        if (keyState[SDL_SCANCODE_LEFT] || keyState[SDL_SCANCODE_A]) {
            centerX -= moveSpeed;
        }
        if (keyState[SDL_SCANCODE_RIGHT] || keyState[SDL_SCANCODE_D]) {
            centerX += moveSpeed;
        }
        if (keyState[SDL_SCANCODE_UP] || keyState[SDL_SCANCODE_W]) {
            centerY += moveSpeed;
        }
        if (keyState[SDL_SCANCODE_DOWN] || keyState[SDL_SCANCODE_S]) {
            centerY -= moveSpeed;
        }
        
    
        if (zoom < 10.0) {
            centerX = std::max(minX, std::min(maxX, centerX));
            centerY = std::max(minY, std::min(maxY, centerY));
        }
        
        if (keyState[SDL_SCANCODE_Z]) {
            zoom *= 1.02;
            if (zoom > 1e14) zoom = 1e14;
        }
        if (keyState[SDL_SCANCODE_X]) {
            zoom /= 1.02;
            if (zoom < 0.5) zoom = 0.5;
        }
        
        SDL_Color white {255, 255, 255, 255};
        SDL_Color yellow {255, 255, 0, 255};
        
        static uint64_t frameCount = 0;
        static Uint64 fpsLastTime = SDL_GetPerformanceCounter();
        static double fps = 0.0;
        static int fpsUpdateCounter = 0;
        
        ++frameCount;
        ++fpsUpdateCounter;
        
        Uint64 fpsCurrentTime = SDL_GetPerformanceCounter();
        double elapsed = (fpsCurrentTime - fpsLastTime) / (double)SDL_GetPerformanceFrequency();
        
        if (fpsUpdateCounter >= 10) {
            fps = fpsUpdateCounter / elapsed;
            fpsLastTime = fpsCurrentTime;
            fpsUpdateCounter = 0;
        }
        
        std::ostringstream infoStream;
        infoStream << std::scientific << std::setprecision(6);
        infoStream << "Center: (" << centerX << ", " << centerY << ")";
        
        std::ostringstream zoomStream;
        zoomStream << std::scientific << std::setprecision(2);
        zoomStream << "Zoom: " << zoom << "x | Iterations: " << maxIterations;
        
        std::ostringstream fpsStream;
        fpsStream << std::fixed << std::setprecision(1) << "FPS: " << fps;
        
        printText("Mandelbrot Fractal", 10, 10, yellow);
        printText(infoStream.str(), 10, 40, white);
        printText(zoomStream.str(), 10, 70, white);
        printText(fpsStream.str(), 10, 100, white);
        printText("Controls: Scroll=Zoom, Drag=Pan, WASD/Arrows=Move, Z/X=Zoom, R=Reset, 1-3=Presets", 10, h - 30, white);
    }
    
    void draw() override {
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw mx::Exception("Failed to acquire swap chain image!");
        }

        VK_CHECK_RESULT(vkResetCommandBuffer(commandBuffers[imageIndex], 0));

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
            throw mx::Exception("Failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent;
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
        vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, fractalPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

        if (vertexBuffer != VK_NULL_HANDLE) {
            VkBuffer vertexBuffers[] = { vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);
        }

        if (indexBuffer != VK_NULL_HANDLE) {
            vkCmdBindIndexBuffer(commandBuffers[imageIndex], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }

        {
            mx::UniformBufferObject ubo{};
            ubo.model = glm::mat4(1.0f);
            ubo.view = glm::mat4(1.0f);
            ubo.proj = glm::mat4(1.0f);
            ubo.params = glm::vec4(static_cast<float>(swapChainExtent.width), 
                                    static_cast<float>(swapChainExtent.height), 0.0f, 0.0f);
            ubo.color = glm::vec4(1.0f);
            
            if (uniformBuffersMapped.size() > imageIndex && uniformBuffersMapped[imageIndex] != nullptr) {
                memcpy(uniformBuffersMapped[imageIndex], &ubo, sizeof(ubo));
            }
        }
        
        if (!descriptorSets.empty()) {
            vkCmdBindDescriptorSets(
                commandBuffers[imageIndex],
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                fractalPipelineLayout,
                0,
                1,
                &descriptorSets[imageIndex],
                0,
                nullptr
            );
        }

        
        FractalPushConstants pc{};
        pc.centerX = static_cast<float>(centerX);
        pc.centerY = static_cast<float>(centerY);
        pc.zoom = static_cast<float>(zoom);
        pc.maxIterations = maxIterations;
        pc.time = animTime;
        
        vkCmdPushConstants(commandBuffers[imageIndex], fractalPipelineLayout,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(FractalPushConstants), &pc);
        
        vkCmdDrawIndexed(commandBuffers[imageIndex], indexCount, 1, 0, 0, 0);
        
        if (!captureNextFrame && textRenderer && textPipeline != VK_NULL_HANDLE) {
            try {
                vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, textPipeline);
                textRenderer->renderText(commandBuffers[imageIndex], textPipelineLayout,
                                       swapChainExtent.width, swapChainExtent.height);
            } catch (const std::exception& e) {
            }
        }
        
        vkCmdEndRenderPass(commandBuffers[imageIndex]);
        if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
            throw mx::Exception("Failed to record command buffer!");
        }
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if (submitResult != VK_SUCCESS) {
            std::cerr << "vkQueueSubmit failed with VkResult: " << submitResult << std::endl;
            throw mx::Exception("Failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            throw mx::Exception("Failed to present swap chain image!");
        }
        
        VK_CHECK_RESULT(vkQueueWaitIdle(presentQueue));
        
        if (captureNextFrame) {
            saveScreenshot(imageIndex);
            captureNextFrame = false;
        }
        
        clearTextQueue();
    }

private:
};

int main(int argc, char **argv) {
#if defined(__APPLE__) || defined(_WIN32) || defined(_WIN64) || defined(__linux__)
#ifndef __ANDROID__
    Arguments args = proc_args(argc, argv);
    try {
        FractalWindow window(args.path, args.width, args.height, args.fullscreen);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
#endif
#elif defined(__ANDROID__)
    try {
        FractalWindow window("", 960, 720, false);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
#endif
    return EXIT_SUCCESS;
}
