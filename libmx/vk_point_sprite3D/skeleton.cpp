#include "vk.hpp"
#include "vk_model.hpp"
#include "loadpng.hpp"
#include "SDL.h"
#include <random>
#include <cmath>
#include "argz.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct LightOrb {
    float x, y, z;
    float vx, vy, vz;
    float r, g, b;
    float brightness;
    float size;
    float pulsePhase;
    float pulseSpeed;
    float hue;
    float minBrightness;
    float maxBrightness;
    float strobeTimer;
    float strobeInterval;
    float sizeSpeed;
    bool dir;

    void update(float dt, float bounceRadius, std::mt19937& rng) {
        x += vx * dt;
        y += vy * dt;
        z += vz * dt;
        float dist = std::sqrt(x * x + y * y + z * z);
        if (dist >= bounceRadius) {
            float nx = x / dist;
            float ny = y / dist;
            float nz = z / dist;
            float dot = vx * nx + vy * ny + vz * nz;
            vx -= 2.0f * dot * nx;
            vy -= 2.0f * dot * ny;
            vz -= 2.0f * dot * nz;
            float pushBack = bounceRadius * 0.95f;
            x = nx * pushBack;
            y = ny * pushBack;
            z = nz * pushBack;
        }

        pulsePhase += pulseSpeed * dt;
        if (pulsePhase > 2.0f * M_PI) pulsePhase -= 2.0f * M_PI;
        float pulseFactor = 0.5f * (1.0f + std::sin(pulsePhase));
        brightness = minBrightness + (maxBrightness - minBrightness) * pulseFactor;

        strobeTimer += dt;
        if (strobeTimer >= strobeInterval) {
            strobeTimer = 0.0f;
            static std::uniform_int_distribution<int> hueDist(0, 359);
            hue = static_cast<float>(hueDist(rng));
            updateColorFromHue();
        }

        if (dir) {
            size += sizeSpeed * dt;
            if (size >= 128.0f) { size = 128.0f; dir = false; }
        } else {
            size -= sizeSpeed * dt;
            if (size <= 64.0f) { size = 64.0f; dir = true; }
        }
    }

    void updateColorFromHue() {
        float h = hue / 60.0f;
        float s = 1.0f;
        float v = 1.0f;
        int i = static_cast<int>(h) % 6;
        float f = h - static_cast<int>(h);
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));
        switch (i) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }
    }
};

class PointSpriteWindow : public mx::VKWindow {
public:
    PointSpriteWindow(const std::string& path, int wx, int wy, bool full)
        : mx::VKWindow("-[ Vulkan Point Sprites - Stars in Sphere ]-", wx, wy, full) {
        setPath(path);
        model.reset(new mx::MXModel());
        model->load(path + "/data/fixed_sphere.mxmod.z", 1.0f);
    }
    virtual ~PointSpriteWindow() {}

    void setFileName(const std::string &filename) {
        this->filename = filename;
    }

    void initVulkan() override {
        mx::VKWindow::initVulkan();
        model->upload(device, physicalDevice, commandPool, graphicsQueue);

        background = createSprite(util.getFilePath("data/universe.png"),
            util.getFilePath("data/sprite_vert.spv"),
            util.getFilePath("data/universe_fragment_frag.spv")
        );

        loadGlassTexture();
        createGlassPipeline();

        initOrbs();
        createOrbSprite();
    }

    void loadGlassTexture() {
        SDL_Surface* surface = png::LoadPNG(util.getFilePath("data/glass.png").c_str());
        if (surface) {
            createTextureImage(surface);
            createTextureImageView();
            updateDescriptorSets();
            SDL_FreeSurface(surface);
        }
    }

    void createGlassPipeline() {
        auto vertShaderCode = mx::readFile(util.getFilePath("data/vert.spv"));
        auto fragShaderCode = mx::readFile(util.getFilePath("data/glass_fragment_frag.spv"));

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertShaderModule;
        vertStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragStage{};
        fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = fragShaderModule;
        fragStage.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };

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
        viewportState.scissorCount = 1;

        std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_FALSE;  
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

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
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

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
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &glassPipeline));

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void initOrbs() {
        std::uniform_real_distribution<float> posDist(-BOUNCE_RADIUS * 0.4f, BOUNCE_RADIUS * 0.4f);
        std::uniform_real_distribution<float> velDist(-0.5f, 0.5f);
        std::uniform_real_distribution<float> sizeDist(20.0f, 60.0f);
        std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * M_PI);
        std::uniform_real_distribution<float> pulseSpeedDist(0.5f, 3.0f);
        std::uniform_real_distribution<float> minBrightDist(0.2f, 0.5f);
        std::uniform_real_distribution<float> maxBrightDist(0.8f, 1.5f);
        std::uniform_int_distribution<int> hueDist(0, 359);
        std::uniform_real_distribution<float> strobeIntervalDist(1.0f, 3.0f);
        std::uniform_real_distribution<float> strobeTimerDist(0.0f, 3.0f);

        orbs.resize(NUM_ORBS);
        for (size_t i = 0; i < orbs.size(); ++i) {
            auto& orb = orbs[i];
            do {
                orb.x = posDist(gen);
                orb.y = posDist(gen);
                orb.z = posDist(gen);
            } while (std::sqrt(orb.x*orb.x + orb.y*orb.y + orb.z*orb.z) > BOUNCE_RADIUS * 0.7f);
            orb.vx = velDist(gen);
            orb.vy = velDist(gen);
            orb.vz = velDist(gen);
            orb.size = sizeDist(gen);
            orb.hue = static_cast<float>(hueDist(gen));
            orb.pulsePhase = phaseDist(gen);
            orb.pulseSpeed = pulseSpeedDist(gen);
            orb.minBrightness = minBrightDist(gen);
            orb.maxBrightness = maxBrightDist(gen);
            orb.strobeInterval = strobeIntervalDist(gen);
            orb.strobeTimer = strobeTimerDist(gen);
            orb.sizeSpeed = 60.0f;
            orb.dir = std::uniform_int_distribution<int>(0, 1)(gen) == 0;
            float pulseFactor = 0.5f * (1.0f + std::sin(orb.pulsePhase));
            orb.brightness = orb.minBrightness + (orb.maxBrightness - orb.minBrightness) * pulseFactor;
            orb.updateColorFromHue();
        }
        lastTime = SDL_GetTicks();
    }

    void createOrbSprite() {
        orbSprite = createSprite(util.path + "/data/mstar.png",
            util.getFilePath("data/sprite_vert.spv"),
            util.getFilePath("data/sprite_fragment_frag.spv")
        );
        orbSprite->enableInstancing(
            1024,
            util.getFilePath("data/sprite_instance_vertex_vert.spv"),
            util.getFilePath("data/sprite_instance_fragment_frag.spv")
        );
    }

    void proc() override {
        Uint32 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        if (dt > 0.05f) dt = 0.05f;

        if (background) {
            float bgTime = SDL_GetTicks() / 1000.0f;
            background->setShaderParams(1.0f, 1.0f, 1.0f, bgTime);
            background->drawSpriteRect(0, 0, getWidth(), getHeight());
        }

        for (auto& orb : orbs) {
            orb.update(dt, BOUNCE_RADIUS, gen);
        }

        float time = SDL_GetTicks() / 1000.0f;
        float aspect = static_cast<float>(w) / static_cast<float>(h);
        glm::mat4 viewMat = glm::lookAt(getCameraPos(),
                                         glm::vec3(0.0f, 0.0f, 0.0f),
                                         glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projMat = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
        projMat[1][1] *= -1;

        glm::mat4 sphereModelMat = glm::mat4(1.0f);
        sphereModelMat = glm::scale(sphereModelMat, glm::vec3(sphereRadius * 1.1f));
        sphereModelMat = glm::rotate(sphereModelMat, time * glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        sphereModelMat = glm::rotate(sphereModelMat, time * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        float currentTimeSec = static_cast<float>(currentTime) / 1000.0f;
        for (const auto& orb : orbs) {
            if (orbSprite) {
                glm::vec4 worldPos = sphereModelMat * glm::vec4(orb.x, orb.y, orb.z, 1.0f);
                glm::vec4 clipPos = projMat * viewMat * worldPos;
                if (clipPos.w <= 0.0f) continue;
                glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
                float screenX = (ndc.x * 0.5f + 0.5f) * w;
                float screenY = (ndc.y * 0.5f + 0.5f) * h;

                float depthScale = 1.0f / clipPos.w;
                float spriteSize = orb.size * depthScale * 2.0f * sphereRadius;
                float scale = spriteSize / 64.0f;
                if (scale < 0.05f) continue;

                orbSprite->setShaderParams(orb.r * orb.brightness, orb.g * orb.brightness, orb.b * orb.brightness, currentTimeSec);
                orbSprite->drawSprite(
                    static_cast<int>(screenX - spriteSize / 2),
                    static_cast<int>(screenY - spriteSize / 2),
                    scale, scale
                );
            }
        }

        if (hudEnabled) {
            printText("-[ Stars Bouncing Inside Wireframe Sphere ]-", 20, 20, {255, 255, 255, 255});
            std::string orbInfo = "Stars: " + std::to_string(orbs.size()) + " | FPS: " + std::to_string(static_cast<int>(1.0f / std::max(dt, 0.001f)));
            printText(orbInfo, 20, 50, {200, 200, 200, 255});
            printText("SPACE: add stars | C: reset | W: wireframe | K: kaleidoscope | ESC: quit", 20, 80, {150, 150, 150, 255});
        }
    }

    void draw() override {
        uint32_t imageIndex = 0;

        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                                 imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
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
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        VkCommandBuffer cmd = commandBuffers[imageIndex];
        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width  = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        
        if (spritePipeline != VK_NULL_HANDLE && !sprites.empty()) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline);
            for (auto& sprite : sprites) {
                if (sprite) {
                    sprite->renderSprites(cmd, spritePipelineLayout,
                                          swapChainExtent.width, swapChainExtent.height);
                }
            }
        }

        float time = SDL_GetTicks() / 1000.0f;
        float aspect = static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);

        VkPipeline modelPipeline = wireframe ? graphicsPipelineMatrix : glassPipeline;
        if (sphereVisible && modelPipeline != VK_NULL_HANDLE && model && model->indexCount() > 0) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, modelPipeline);

            mx::UniformBufferObject ubo{};
            glm::mat4 modelMat = glm::mat4(1.0f);
            modelMat = glm::scale(modelMat, glm::vec3(sphereRadius * 1.1f));
            modelMat = glm::rotate(modelMat, time * glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            modelMat = glm::rotate(modelMat, time * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            ubo.model = modelMat;
            ubo.view = glm::lookAt(getCameraPos(),
                                    glm::vec3(0.0f, 0.0f, 0.0f),
                                    glm::vec3(0.0f, 1.0f, 0.0f));
            ubo.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
            ubo.proj[1][1] *= -1;
            ubo.params = glm::vec4(time, static_cast<float>(swapChainExtent.width),
                                   static_cast<float>(swapChainExtent.height), kaleidoEnabled ? 1.0f : 0.0f);
            ubo.color = glm::vec4(0.2f, 0.8f, 1.0f, 0.4f);

            memcpy(uniformBuffersMapped[imageIndex], &ubo, sizeof(ubo));

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelineLayout, 0, 1, &descriptorSets[imageIndex], 0, nullptr);
            model->draw(cmd);
        }

        if (textRenderer && textPipeline != VK_NULL_HANDLE) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, textPipeline);
            textRenderer->renderText(cmd, textPipelineLayout,
                                     swapChainExtent.width, swapChainExtent.height);
        }

        vkCmdEndRenderPass(cmd);

        if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
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
        submitInfo.pCommandBuffers = &cmd;
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if (submitResult != VK_SUCCESS) {
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
        } else if (result != VK_SUCCESS) {
            throw mx::Exception("Failed to present swap chain image!");
        }

        clearTextQueue();
        for (auto& sprite : sprites) {
            if (sprite) {
                sprite->clearQueue();
            }
        }
    }

    void cleanup() override {
        vkDeviceWaitIdle(device);
        if (glassPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, glassPipeline, nullptr);
            glassPipeline = VK_NULL_HANDLE;
        }
        if (model) {
            model->cleanup(device);
        }
        mx::VKWindow::cleanup();
    }

    bool hudEnabled = true;

    void event(SDL_Event& e) override {
        if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
            quit();
            return;
        }
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_SPACE) {
                addRandomOrbs(50);
            }
            if (e.key.keysym.sym == SDLK_c) {
                orbs.clear();
                initOrbs();
            }
            if (e.key.keysym.sym == SDLK_v) {
                hudEnabled = !hudEnabled;
            }
            if (e.key.keysym.sym == SDLK_w) {
                wireframe = !wireframe;
            }
            if (e.key.keysym.sym == SDLK_h) {
                sphereVisible = !sphereVisible;
            }
            if (e.key.keysym.sym == SDLK_k) {
                kaleidoEnabled = !kaleidoEnabled;
            }
            if (e.key.keysym.sym == SDLK_UP) {
                sphereRadius += 0.1f;
                if (sphereRadius > 5.0f) sphereRadius = 5.0f;
            }
            if (e.key.keysym.sym == SDLK_DOWN) {
                sphereRadius -= 0.1f;
                if (sphereRadius < 0.3f) sphereRadius = 0.3f;
            }
            if (e.key.keysym.sym == SDLK_LEFT) {
                cameraAngleY -= 0.1f;
            }
            if (e.key.keysym.sym == SDLK_RIGHT) {
                cameraAngleY += 0.1f;
            }
            if (e.key.keysym.sym == SDLK_PAGEUP) {
                cameraAngleX -= 0.1f;
                if (cameraAngleX < -1.5f) cameraAngleX = -1.5f;
            }
            if (e.key.keysym.sym == SDLK_PAGEDOWN) {
                cameraAngleX += 0.1f;
                if (cameraAngleX > 1.5f) cameraAngleX = 1.5f;
            }
        }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            mouseDragging = true;
            lastMouseX = e.button.x;
            lastMouseY = e.button.y;
        }
        if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
            mouseDragging = false;
        }
        if (e.type == SDL_MOUSEMOTION && mouseDragging) {
            int dx = e.motion.x - lastMouseX;
            int dy = e.motion.y - lastMouseY;
            cameraAngleY += dx * 0.005f;
            cameraAngleX += dy * 0.005f;
            if (cameraAngleX < -1.5f) cameraAngleX = -1.5f;
            if (cameraAngleX > 1.5f) cameraAngleX = 1.5f;
            lastMouseX = e.motion.x;
            lastMouseY = e.motion.y;
        }
    }

    void addRandomOrbs(int count) {
        std::uniform_real_distribution<float> posDist(-BOUNCE_RADIUS * 0.4f, BOUNCE_RADIUS * 0.4f);
        std::uniform_real_distribution<float> velDist(-0.5f, 0.5f);
        std::uniform_real_distribution<float> sizeDist(20.0f, 60.0f);
        std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * M_PI);
        std::uniform_real_distribution<float> pulseSpeedDist(0.5f, 3.0f);
        std::uniform_real_distribution<float> minBrightDist(0.2f, 0.5f);
        std::uniform_real_distribution<float> maxBrightDist(0.8f, 1.5f);
        std::uniform_int_distribution<int> hueDist(0, 359);
        std::uniform_real_distribution<float> strobeIntervalDist(1.0f, 3.0f);
        std::uniform_real_distribution<float> strobeTimerDist(0.0f, 3.0f);

        for (int i = 0; i < count; i++) {
            LightOrb orb;
            do {
                orb.x = posDist(gen);
                orb.y = posDist(gen);
                orb.z = posDist(gen);
            } while (std::sqrt(orb.x*orb.x + orb.y*orb.y + orb.z*orb.z) > BOUNCE_RADIUS * 0.7f);
            orb.vx = velDist(gen);
            orb.vy = velDist(gen);
            orb.vz = velDist(gen);
            orb.size = sizeDist(gen);
            orb.dir = std::uniform_int_distribution<int>(0, 1)(gen) == 0;
            orb.hue = static_cast<float>(hueDist(gen));
            orb.pulsePhase = phaseDist(gen);
            orb.pulseSpeed = pulseSpeedDist(gen);
            orb.minBrightness = minBrightDist(gen);
            orb.maxBrightness = maxBrightDist(gen);
            orb.strobeInterval = strobeIntervalDist(gen);
            orb.strobeTimer = strobeTimerDist(gen);
            orb.sizeSpeed = 60.0f;
            float pulseFactor = 0.5f * (1.0f + std::sin(orb.pulsePhase));
            orb.brightness = orb.minBrightness + (orb.maxBrightness - orb.minBrightness) * pulseFactor;
            orb.updateColorFromHue();
            orbs.push_back(orb);
        }
    }

private:
    static constexpr int NUM_ORBS = 50;
    static constexpr float BOUNCE_RADIUS = 0.9f;
    std::vector<LightOrb> orbs;
    mx::VKSprite *orbSprite = nullptr;
    mx::VKSprite *background = nullptr;
    VkPipeline glassPipeline = VK_NULL_HANDLE;
    std::unique_ptr<mx::MXModel> model;
    bool wireframe = true;
    bool sphereVisible = true;
    bool kaleidoEnabled = false;
    float sphereRadius = 1.0f;
    float cameraAngleY = 0.0f;
    float cameraAngleX = 0.0f;
    bool mouseDragging = false;
    int lastMouseX = 0;
    int lastMouseY = 0;
    Uint32 lastTime = 0;
    std::string filename;
    std::random_device rd;
    std::mt19937 gen{rd()};

    glm::vec3 getCameraPos() const {
        float cx = cameraDistance * std::sin(cameraAngleY) * std::cos(cameraAngleX);
        float cy = cameraDistance * std::sin(cameraAngleX);
        float cz = cameraDistance * std::cos(cameraAngleY) * std::cos(cameraAngleX);
        return glm::vec3(cx, cy, cz);
    }
};

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
        PointSpriteWindow window(args.path, args.width, args.height, args.fullscreen);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
    return EXIT_SUCCESS;
}
