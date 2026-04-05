#include "SDL.h"
#include "argz.hpp"
#include "loadpng.hpp"
#include "vk.hpp"
#include "vk_model.hpp"
#include <array>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream>

class TuxWindow : public mx::VKWindow {
  public:
    TuxWindow(const std::string &path, int wx, int wy, bool full)
        : mx::VKWindow("-[ Vulkan Tux Model ]-", wx, wy, full) {
        setPath(path);
    }

    void initVulkan() override {
        mx::VKWindow::initVulkan();

        sprite = createSprite(util.path + "/data/ant-bg.png",
                              util.path + "/data/sprite_vert.spv",
                              util.path + "/data/fractal_texture_large.spv");

        model.load(util.path + "/data/tux.obj", 1.0f);
        const auto &verts = model.vertices();
        if (!verts.empty()) {
            float minX = verts[0].pos[0], maxX = minX;
            float minY = verts[0].pos[1], maxY = minY;
            float minZ = verts[0].pos[2], maxZ = minZ;
            for (const auto &v : verts) {
                minX = std::min(minX, v.pos[0]);
                maxX = std::max(maxX, v.pos[0]);
                minY = std::min(minY, v.pos[1]);
                maxY = std::max(maxY, v.pos[1]);
                minZ = std::min(minZ, v.pos[2]);
                maxZ = std::max(maxZ, v.pos[2]);
            }
            modelCenterOffset = glm::vec3(
                -0.5f * (minX + maxX),
                -0.5f * (minY + maxY),
                -0.5f * (minZ + maxZ));
            float maxExtent = std::max(maxX - minX, std::max(maxY - minY, maxZ - minZ));
            if (maxExtent > 1e-6f)
                modelRenderScale = 2.5f / maxExtent;
        }

        model.upload(device, physicalDevice, commandPool, graphicsQueue);
        loadModelTextures();
        createModelDescriptorPool();
        createModelDescriptorSets();
    }

    void proc() override {
        if (sprite) {
            float time_f = SDL_GetTicks() / 1000.0f;
            sprite->setShaderParams(time_f, mouseX, mouseY, mousePressed ? 1.0f : 0.0f);
            sprite->drawSpriteRect(0, 0, getWidth(), getHeight());
        }

        std::string info = "Tux Model - Arrow keys/mouse to rotate, +/- zoom, W wireframe, R auto-rotate";
        printText(info, 10, 10, {255, 255, 255, 255});
    }

    void draw() override {
        uint32_t imageIndex = 0;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                                imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw mx::Exception("Failed to acquire swap chain image!");

        vkResetFences(device, 1, &inFlightFence);
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
        clearValues[0].color = {{0.05f, 0.05f, 0.1f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        rpInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        rpInfo.pClearValues = clearValues.data();

        VkCommandBuffer cmd = commandBuffers[imageIndex];
        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{{0, 0}, swapChainExtent};
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        if (spritePipeline != VK_NULL_HANDLE && !sprites.empty()) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline);
            for (auto &s : sprites) {
                if (s)
                    s->renderSprites(cmd, spritePipelineLayout,
                                     swapChainExtent.width, swapChainExtent.height);
            }
        }

        VkPipeline activePipeline = (useWireFrame && graphicsPipelineMatrix != VK_NULL_HANDLE)
                                        ? graphicsPipelineMatrix
                                        : graphicsPipeline;
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, activePipeline);

        mx::UniformBufferObject ubo{};
        float time = SDL_GetTicks() / 1000.0f;
        float dt = time - lastTime;
        lastTime = time;
        if (autoRotate)
            autoAngle += dt * glm::radians(45.0f);

        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::scale(modelMat, glm::vec3(modelRenderScale));
        modelMat = glm::rotate(modelMat, glm::radians(rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMat = glm::rotate(modelMat, glm::radians(rotationY) + autoAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::translate(modelMat, modelCenterOffset);

        float aspect = static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
        ubo.model = modelMat;
        ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, camDist), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.0f);
        ubo.proj[1][1] *= -1; // Vulkan Y-flip
        ubo.params = glm::vec4(time, (float)swapChainExtent.width, (float)swapChainExtent.height, 0.0f);
        ubo.color = glm::vec4(1.0f);

        if (imageIndex < uniformBuffersMapped.size() && uniformBuffersMapped[imageIndex])
            memcpy(uniformBuffersMapped[imageIndex], &ubo, sizeof(ubo));

        const size_t texCount = std::max<size_t>(1, modelTextures.size());
        if (model.subMeshCount() > 0) {
            for (size_t i = 0; i < model.subMeshCount(); ++i) {
                size_t texIdx = std::min<size_t>(model.subMesh(i).textureIndex, texCount - 1);
                size_t setIdx = static_cast<size_t>(imageIndex) * texCount + texIdx;
                if (setIdx < modelDescriptorSets.size()) {
                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            pipelineLayout, 0, 1, &modelDescriptorSets[setIdx], 0, nullptr);
                }
                model.drawSubMesh(cmd, i);
            }
        } else {
            size_t texIdx = std::min<size_t>(model.textureIndex(), texCount - 1);
            size_t setIdx = static_cast<size_t>(imageIndex) * texCount + texIdx;
            if (setIdx < modelDescriptorSets.size()) {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        pipelineLayout, 0, 1, &modelDescriptorSets[setIdx], 0, nullptr);
            }
            model.draw(cmd);
        }

        if (textRenderer && textPipeline != VK_NULL_HANDLE) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, textPipeline);
            textRenderer->renderText(cmd, textPipelineLayout,
                                     swapChainExtent.width, swapChainExtent.height);
        }

        vkCmdEndRenderPass(cmd);

        if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
            throw mx::Exception("Failed to record command buffer!");

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSems[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSems;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        VkSemaphore signalSems[] = {renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSems;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS)
            throw mx::Exception("Failed to submit draw command buffer!");

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSems;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = &imageIndex;
        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw mx::Exception("Failed to present swap chain image!");
        }

        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        clearTextQueue();
        for (auto &s : sprites) {
            if (s)
                s->clearQueue();
        }
    }

    void event(SDL_Event &e) override {
        if (e.type == SDL_QUIT) {
            quit();
            return;
        }
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
            case SDLK_ESCAPE:
                quit();
                break;
            case SDLK_w:
                useWireFrame = !useWireFrame;
                break;
            case SDLK_r:
                autoRotate = !autoRotate;
                break;
            case SDLK_UP:
                rotationX -= 5.0f;
                break;
            case SDLK_DOWN:
                rotationX += 5.0f;
                break;
            case SDLK_LEFT:
                rotationY -= 5.0f;
                break;
            case SDLK_RIGHT:
                rotationY += 5.0f;
                break;
            case SDLK_EQUALS:
            case SDLK_PLUS:
                camDist = std::max(0.1f, camDist - 0.5f);
                break;
            case SDLK_MINUS:
                camDist = std::min(100.0f, camDist + 0.5f);
                break;
            case SDLK_HOME:
                rotationX = 0;
                rotationY = 0;
                camDist = 5.0f;
                autoAngle = 0;
                break;
            }
        }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            mouseDown = true;
            mousePressed = true;
            lastMouseX = e.button.x;
            lastMouseY = e.button.y;
            mouseX = static_cast<float>(e.button.x);
            mouseY = static_cast<float>(e.button.y);
        }
        if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
            mouseDown = false;
            mousePressed = false;
        }
        if (e.type == SDL_MOUSEMOTION) {
            mouseX = static_cast<float>(e.motion.x);
            mouseY = static_cast<float>(e.motion.y);
            if (mouseDown) {
                rotationY += (e.motion.x - lastMouseX) * 0.5f;
                rotationX += (e.motion.y - lastMouseY) * 0.5f;
                rotationX = glm::clamp(rotationX, -89.0f, 89.0f);
                lastMouseX = e.motion.x;
                lastMouseY = e.motion.y;
            }
        }
        if (e.type == SDL_MOUSEWHEEL) {
            camDist -= e.wheel.y * 0.5f;
            camDist = glm::clamp(camDist, 0.1f, 100.0f);
        }
    }

    void onResize() override {
        if (modelDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, modelDescriptorPool, nullptr);
            modelDescriptorPool = VK_NULL_HANDLE;
        }
        modelDescriptorSets.clear();
        createModelDescriptorPool();
        createModelDescriptorSets();
    }

    void cleanup() override {
        vkDeviceWaitIdle(device);
        model.cleanup(device);
        for (auto &t : modelTextures) {
            if (t.view != VK_NULL_HANDLE)
                vkDestroyImageView(device, t.view, nullptr);
            if (t.image != VK_NULL_HANDLE)
                vkDestroyImage(device, t.image, nullptr);
            if (t.memory != VK_NULL_HANDLE)
                vkFreeMemory(device, t.memory, nullptr);
        }
        modelTextures.clear();
        if (modelDescriptorPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(device, modelDescriptorPool, nullptr);
        mx::VKWindow::cleanup();
    }

  private:
    struct TexEntry {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        uint32_t w = 0, h = 0;
    };

    void loadModelTextures() {
        std::string prefix = util.path + "/data";
        std::vector<std::string> imagePaths;

        // Use material info from the loaded OBJ/MTL
        const auto &materials = model.materials();
        if (!materials.empty()) {
            for (const auto &mat : materials) {
                if (!mat.map_kd.empty())
                    imagePaths.push_back(prefix + "/" + mat.map_kd);
            }
        }

        // Fallback to .tex file if no MTL materials found
        if (imagePaths.empty()) {
            std::string texPath = util.getFilePath("data/tux.tex");
            std::ifstream tf(texPath);
            if (!tf.is_open())
                throw mx::Exception("Failed to open .tex file: " + texPath);

            std::string line;
            while (std::getline(tf, line)) {
                size_t b = line.find_first_not_of(" \t\r\n");
                if (b == std::string::npos)
                    continue;
                size_t e = line.find_last_not_of(" \t\r\n");
                line = line.substr(b, e - b + 1);
                if (line.empty() || line[0] == '#')
                    continue;
                imagePaths.push_back(prefix + "/" + line);
            }
        }

        if (imagePaths.empty())
            throw mx::Exception("No textures found from MTL or .tex file");

        VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
        for (size_t i = 0; i < imagePaths.size(); ++i) {
            SDL_Surface *img = png::LoadPNG(imagePaths[i].c_str());
            if (!img) {
                img = SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 32, SDL_PIXELFORMAT_RGBA32);
                if (img) {
                    uint32_t *px = (uint32_t *)img->pixels;
                    *px = 0xFFFFFFFF;
                } else
                    throw mx::Exception("Failed to create placeholder surface");
            }

            TexEntry tex;
            tex.w = img->w;
            tex.h = img->h;
            VkDeviceSize imageSize = tex.w * tex.h * 4;

            createImage(tex.w, tex.h, fmt, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex.image, tex.memory);

            transitionImageLayout(tex.image, fmt, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkBuffer staging;
            VkDeviceMemory stagingMem;
            createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         staging, stagingMem);
            void *data;
            vkMapMemory(device, stagingMem, 0, imageSize, 0, &data);
            memcpy(data, img->pixels, (size_t)imageSize);
            vkUnmapMemory(device, stagingMem);
            copyBufferToImage(staging, tex.image, tex.w, tex.h);

            transitionImageLayout(tex.image, fmt, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            vkDestroyBuffer(device, staging, nullptr);
            vkFreeMemory(device, stagingMem, nullptr);
            SDL_FreeSurface(img);

            tex.view = createImageView(tex.image, fmt, VK_IMAGE_ASPECT_COLOR_BIT);
            modelTextures.push_back(tex);
        }
    }

    void createModelDescriptorPool() {
        const uint32_t texCount = std::max<uint32_t>(1u, static_cast<uint32_t>(modelTextures.size()));
        const uint32_t setCount = static_cast<uint32_t>(swapChainImages.size()) * texCount;

        std::array<VkDescriptorPoolSize, 2> sizes{};
        sizes[0] = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, setCount};
        sizes[1] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, setCount};

        VkDescriptorPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ci.poolSizeCount = static_cast<uint32_t>(sizes.size());
        ci.pPoolSizes = sizes.data();
        ci.maxSets = setCount;
        if (vkCreateDescriptorPool(device, &ci, nullptr, &modelDescriptorPool) != VK_SUCCESS)
            throw mx::Exception("Failed to create model descriptor pool!");
    }

    void createModelDescriptorSets() {
        const size_t texCount = std::max<size_t>(1, modelTextures.size());
        const size_t setCount = swapChainImages.size() * texCount;

        std::vector<VkDescriptorSetLayout> layouts(setCount, descriptorSetLayout);
        VkDescriptorSetAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool = modelDescriptorPool;
        ai.descriptorSetCount = static_cast<uint32_t>(setCount);
        ai.pSetLayouts = layouts.data();
        modelDescriptorSets.resize(setCount);
        if (vkAllocateDescriptorSets(device, &ai, modelDescriptorSets.data()) != VK_SUCCESS)
            throw mx::Exception("Failed to allocate model descriptor sets!");

        for (size_t frame = 0; frame < swapChainImages.size(); ++frame) {
            VkDescriptorBufferInfo bufInfo{};
            bufInfo.buffer = uniformBuffers[frame];
            bufInfo.offset = 0;
            bufInfo.range = sizeof(mx::UniformBufferObject);

            for (size_t tex = 0; tex < texCount; ++tex) {
                size_t setIndex = frame * texCount + tex;

                VkDescriptorImageInfo imgInfo{};
                imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imgInfo.imageView = modelTextures[tex].view;
                imgInfo.sampler = textureSampler;

                std::array<VkWriteDescriptorSet, 2> writes{};
                writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[0].dstSet = modelDescriptorSets[setIndex];
                writes[0].dstBinding = 0;
                writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writes[0].descriptorCount = 1;
                writes[0].pImageInfo = &imgInfo;

                writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[1].dstSet = modelDescriptorSets[setIndex];
                writes[1].dstBinding = 1;
                writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                writes[1].descriptorCount = 1;
                writes[1].pBufferInfo = &bufInfo;

                vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()),
                                       writes.data(), 0, nullptr);
            }
        }
    }

    mx::VKSprite *sprite = nullptr;
    mx::MXModel model;

    std::vector<TexEntry> modelTextures;
    VkDescriptorPool modelDescriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> modelDescriptorSets;

    float rotationX = 0.0f, rotationY = 0.0f;
    float camDist = 5.0f;
    float autoAngle = 0.0f;
    float lastTime = 0.0f;
    float modelRenderScale = 1.0f;
    glm::vec3 modelCenterOffset = glm::vec3(0.0f);
    bool autoRotate = true;
    bool mouseDown = false;
    bool mousePressed = false;
    float mouseX = 0.0f, mouseY = 0.0f;
    int lastMouseX = 0, lastMouseY = 0;

    void textQuads_clear() {
        clearTextQueue();
        for (auto &s : sprites) {
            if (s)
                s->clearQueue();
        }
    }
};

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
        TuxWindow window(args.path, args.width, args.height, args.fullscreen);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
