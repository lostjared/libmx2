#include "vk.hpp"
#include "vk_abstract_model.hpp"
#include "loadpng.hpp"
#include "SDL.h"
#include "argz.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <sstream>
#include <array>

class TuxWindow : public mx::VKWindow {
public:
    TuxWindow(const std::string& path, int wx, int wy, bool full)
        : mx::VKWindow("-[ Vulkan Tux Model ]-", wx, wy, full) {
        setPath(path);
    }

    void initVulkan() override {
        mx::VKWindow::initVulkan();

        sprite = createSprite(util.path + "/data/ant-bg.png",
                              util.path + "/data/sprite_vert.spv",
                              util.path + "/data/fractal_texture_large.spv");

        model[0].load(this, util.path + "/data/tux.obj", util.path + "/data/tux.tex", util.path + "/data/", 1.0f);
        model[0].setShaders(this, util.path + "/data/vert.spv", util.path + "/data/frag.spv");
        for(int i = 1; i < 3; ++i)
            model[i].loadInstance(this, model[0]);
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
        viewport.width  = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{{0, 0}, swapChainExtent};
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        if (spritePipeline != VK_NULL_HANDLE && !sprites.empty()) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline);
            for (auto &s : sprites) {
                if (s) s->renderSprites(cmd, spritePipelineLayout,
                                        swapChainExtent.width, swapChainExtent.height);
            }
        }


        VkPipeline defaultPipeline = (useWireFrame && graphicsPipelineMatrix != VK_NULL_HANDLE)
                                    ? graphicsPipelineMatrix : graphicsPipeline;


        VkPipeline sharedPipeline = VK_NULL_HANDLE;
        if (model[0].hasCustomShaders()) {
            sharedPipeline = (useWireFrame && model[0].getModelPipelineWireframe() != VK_NULL_HANDLE)
                             ? model[0].getModelPipelineWireframe() : model[0].getModelPipeline();
        }

        float time = SDL_GetTicks() / 1000.0f;
        float dt = time - lastTime;
        lastTime = time;
        if (autoRotate)
            autoAngle += dt * glm::radians(45.0f);


        const float rotSpeed  = 90.0f;  
        const float zoomSpeed = 3.0f;   
        const Uint8 *keys = SDL_GetKeyboardState(nullptr);
        if (keys[SDL_SCANCODE_UP])     
            rotationX -= rotSpeed * dt;
        if (keys[SDL_SCANCODE_DOWN])
            rotationX += rotSpeed * dt;
        if (keys[SDL_SCANCODE_LEFT])
           rotationY -= rotSpeed * dt;
        if (keys[SDL_SCANCODE_RIGHT])
          rotationY += rotSpeed * dt;
        if (keys[SDL_SCANCODE_EQUALS] || keys[SDL_SCANCODE_KP_PLUS])
            camDist = std::max(0.1f, camDist - zoomSpeed * dt);
        if (keys[SDL_SCANCODE_MINUS]  || keys[SDL_SCANCODE_KP_MINUS])
            camDist = std::min(100.0f, camDist + zoomSpeed * dt);

        float aspect = static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, camDist),
                                     glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.0f);
        proj[1][1] *= -1;  // Vulkan Y-flip

        const float spacing = 2.3f;
        const float offsets[3] = { -spacing, 0.0f, spacing };


        for (int m = 0; m < 3; ++m) {
            mx::VKAbstractModel &mdl = model[m];

            glm::mat4 modelMat = glm::mat4(1.0f);
            modelMat = glm::translate(modelMat, glm::vec3(offsets[m], 0.0f, 0.0f));
            modelMat = glm::scale(modelMat, glm::vec3(mdl.modelRenderScale));
            modelMat = glm::rotate(modelMat, glm::radians(rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
            
            switch(m) {
                case 0:
                    modelMat = glm::rotate(modelMat, glm::radians(rotationY) + autoAngle, glm::vec3(1.0f, 0.0f, 0.0f));
                    break;
                case 1: 
                    modelMat = glm::rotate(modelMat, glm::radians(rotationY) + autoAngle, glm::vec3(0.0f, 1.0f, 0.0f));
                    break;
                case 2:
                    modelMat = glm::rotate(modelMat, glm::radians(rotationY) + autoAngle, glm::vec3(0.0f, 0.0f, 1.0f));        
                break;
            }
            modelMat = glm::translate(modelMat, mdl.modelCenterOffset);

            mx::UniformBufferObject ubo{};
            ubo.model = modelMat;
            ubo.view  = view;
            ubo.proj  = proj;
            ubo.params = glm::vec4(time, (float)swapChainExtent.width, (float)swapChainExtent.height, 0.0f);
            ubo.color = glm::vec4(1.0f);
            mdl.updateUBO(this, imageIndex, ubo);

            
            VkPipeline activePipeline = (sharedPipeline != VK_NULL_HANDLE)
                                         ? sharedPipeline : defaultPipeline;
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, activePipeline);

            mx::VKAbstractModel &geom = model[0];
            const size_t texCount = std::max<size_t>(1, mdl.modelTextures.size());
            if (geom.obj.subMeshCount() > 0) {
                for (size_t i = 0; i < geom.obj.subMeshCount(); ++i) {
                    size_t texIdx = std::min<size_t>(geom.obj.subMesh(i).textureIndex, texCount - 1);
                    size_t setIdx = static_cast<size_t>(imageIndex) * texCount + texIdx;
                    if (setIdx < mdl.modelDescriptorSets.size()) {
                        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                pipelineLayout, 0, 1, &mdl.modelDescriptorSets[setIdx], 0, nullptr);
                    }
                    geom.obj.drawSubMesh(cmd, i);
                }
            } else {
                size_t texIdx = std::min<size_t>(geom.obj.textureIndex(), texCount - 1);
                size_t setIdx = static_cast<size_t>(imageIndex) * texCount + texIdx;
                if (setIdx < mdl.modelDescriptorSets.size()) {
                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            pipelineLayout, 0, 1, &mdl.modelDescriptorSets[setIdx], 0, nullptr);
                }
                geom.obj.draw(cmd);
            }
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
            if (s) s->clearQueue();
        }
    }

    void event(SDL_Event& e) override {
        if (e.type == SDL_QUIT) { quit(); return; }
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE: quit(); break;
                case SDLK_w: useWireFrame = !useWireFrame; break;
                case SDLK_r: autoRotate = !autoRotate; break;
                case SDLK_HOME:
                    rotationX = 0; rotationY = 0; camDist = 5.0f; autoAngle = 0; break;
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
        for(int i = 0; i < 3; ++i)
            model[i].resize(this);
    }

    void cleanup() override {
        // Clean up instances first, then the source model that owns geometry/textures
        for(int i = 2; i >= 0; --i)
            model[i].cleanup(this);
        mx::VKWindow::cleanup();
    }

private:
    mx::VKSprite *sprite = nullptr;
    mx::VKAbstractModel model[3];
    float rotationX = 0.0f, rotationY = 0.0f;
    float camDist = 5.0f;
    float autoAngle = 0.0f;
    float lastTime = 0.0f;
   
    bool autoRotate = true;
    bool mouseDown = false;
    bool mousePressed = false;
    float mouseX = 0.0f, mouseY = 0.0f;
    int lastMouseX = 0, lastMouseY = 0;

    void textQuads_clear() {
        clearTextQueue();
        for (auto &s : sprites) {
            if (s) s->clearQueue();
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
