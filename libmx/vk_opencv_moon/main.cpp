#include "vk.hpp"
#include "vk_model.hpp"
#include "SDL.h"
#include "argz.hpp"
#include<opencv2/opencv.hpp>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include "loadpng.hpp"
#include <random>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static constexpr int NUM_STARS = 15000;

struct Star {
    float x, y, z;
    float vx, vy, vz;
    float magnitude;
    float temperature;
    float twinkle;
    float size;
    float sizeFactor;
    int starType;
    bool active;
    int textureIndex;
};

struct StarVertex {
    float pos[3];
    float size;
    float color[4];
};

static float randFloat(float mn, float mx) {
    static std::random_device rd;
    static std::default_random_engine eng(rd());
    std::uniform_real_distribution<float> dist(mn, mx);
    return dist(eng);
}

static glm::vec3 starColor(float temp) {
    float r, g, b;
    if (temp < 3700) { r=1; g=temp/3700*0.6f; b=0; }
    else if (temp < 5200) { r=1; g=0.6f+(temp-3700)/1500*0.4f; b=(temp-3700)/1500*0.3f; }
    else if (temp < 6000) { r=1; g=1; b=(temp-5200)/800*0.7f; }
    else if (temp < 7500) { r=1; g=1; b=0.7f+(temp-6000)/1500*0.3f; }
    else { r=0.7f-(temp-7500)/10000*0.4f; g=0.8f+(temp-7500)/10000*0.2f; b=1; }
    return glm::vec3(r, g, b);
}

static float magToSize(float mag) { return glm::clamp(20.0f - mag * 3.0f, 2.0f, 35.0f); }
static float magToAlpha(float mag) { return glm::clamp((6.5f - mag) / 6.5f, 0.0f, 1.0f); }


class Moon : public mx::VKWindow {
public:
    Moon(const std::string& path, int wx, int wy, bool full)
        : mx::VKWindow("-[ Vulkan Example / Texture Mapped Sphere ]-", wx, wy, full) {
        setPath(path);
        model.load(path + "/data/fixed_sphere.mxmod.z", 1.0);
        if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0) {
            for (int i = 0; i < SDL_NumJoysticks(); ++i) {
                if (SDL_IsGameController(i)) {
                    controller = SDL_GameControllerOpen(i);
                    if (controller) {
                        SDL_Log("Controller connected: %s", SDL_GameControllerName(controller));
                        break;
                    }
                }
            }
        }
    }

    void initVulkan() override {
        mx::VKWindow::initVulkan();
        model.upload(device, physicalDevice, commandPool, graphicsQueue);
        initStars();
        initStarResources();
    }

    void openCamera(int index, int width, int height) {
        cap.open(index);
        if(!cap.isOpened()) {
            throw mx::Exception("Error could not open camera at index: " + std::to_string(index));
        }
        cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
        camera_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        camera_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        setupTextureImage(camera_width, camera_height);
        createTextureImageView();
        updateDescriptorSets();
    }
    
    void openFile(const std::string &filename) {
        this->filename = filename;
        if(!cap.open(filename)) {
            throw mx::Exception("Error could not open file: " + filename);
        }
        camera_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        camera_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        setupTextureImage(camera_width, camera_height);
        createTextureImageView();
        updateDescriptorSets();
    }
    
    void proc() override {
        if (controller) {
            float dt = 1.0f / 60.0f;

            float lx = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
            float ly = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
            if (std::abs(lx) > STICK_DEADZONE)
                rotationY += (lx / 32767.0f) * STICK_SENSITIVITY * dt;
            if (std::abs(ly) > STICK_DEADZONE)
                rotationX += (ly / 32767.0f) * STICK_SENSITIVITY * dt;
            float ry = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY);
            if (std::abs(ry) > STICK_DEADZONE) {
                cameraDistance += (ry / 32767.0f) * ZOOM_SENSITIVITY * dt;
                cameraDistance = glm::clamp(cameraDistance, 1.0f, 20.0f);
            }
            float lt = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
            float rt = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
            if (lt > 3000) {
                cameraDistance += (lt / 32767.0f) * ZOOM_SENSITIVITY * dt;
                cameraDistance = glm::clamp(cameraDistance, 1.0f, 20.0f);
            }
            if (rt > 3000) {
                cameraDistance -= (rt / 32767.0f) * ZOOM_SENSITIVITY * dt;
                cameraDistance = glm::clamp(cameraDistance, 1.0f, 20.0f);
            }
        }

        cv::Mat frame;
        if(!cap.read(frame)) {
            if(!filename.empty()) {
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                if(!cap.read(frame)) {
                    quit();
                    return;
                }
            } else {
                quit();
                return;
            }
        }

        cv::Mat rgba;
        cv::cvtColor(frame, rgba, cv::COLOR_BGR2RGBA);
        VkDeviceSize imageSize = rgba.cols * rgba.rows * 4;
        updateTexture(rgba.data, imageSize);
        printText("LostSideDead", 15, 15, {255, 0, 150, 255});
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

        float time = SDL_GetTicks() / 1000.0f;
        float aspect = static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);

        
        glm::mat4 viewMat = glm::lookAt(glm::vec3(0.0f, 0.0f, cameraDistance),
                                         glm::vec3(0.0f, 0.0f, 0.0f),
                                         glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projMat = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
        projMat[1][1] *= -1;

        
        if (graphicsPipeline != VK_NULL_HANDLE && model.indexCount() > 0) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            mx::UniformBufferObject ubo{};
            glm::mat4 modelMat = glm::mat4(1.0f);
            modelMat = glm::scale(modelMat, glm::vec3(1.1f));
            modelMat = glm::rotate(modelMat, glm::radians(rotationX) + time * glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            modelMat = glm::rotate(modelMat, glm::radians(rotationY) + time * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            //modelMat = glm::rotate(modelMat, time * glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.model = modelMat;
            ubo.view = viewMat;
            ubo.proj = projMat;
            ubo.params = glm::vec4(time, static_cast<float>(swapChainExtent.width),
                                   static_cast<float>(swapChainExtent.height), static_cast<float>(effectIndex));
            ubo.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

            memcpy(uniformBuffersMapped[imageIndex], &ubo, sizeof(ubo));

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelineLayout, 0, 1, &descriptorSets[imageIndex], 0, nullptr);
            model.draw(cmd);
        }

        
        drawStars(cmd, imageIndex, viewMat, projMat, time);

        
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
    }

    

    void initStar(Star& star, bool randomZ = true) {
        for (int attempts = 0; attempts < 16; ++attempts) {
            float theta = randFloat(0.0f, 2.0f * M_PI);
            star.z = randomZ ? randFloat(-300.0f, -10.0f) : -300.0f;
            float r = sqrt(randFloat(0.0f, 1.0f)) * 150.0f;
            star.x = r * cos(theta);
            star.y = r * sin(theta);
            
            float d2 = star.x*star.x + star.y*star.y + star.z*star.z;
            if (d2 > starExclusionRadius * starExclusionRadius) break;
        }
        star.active = true;
        star.vx = randFloat(-1.0f, 1.0f);
        star.vy = randFloat(-1.0f, 1.0f);
        star.vz = 175.0f;

        float rnd = randFloat(0.0f, 1.0f);
        if (rnd < 0.05f) { star.magnitude = randFloat(-1.5f, 1.5f); star.starType = 0; }
        else if (rnd < 0.25f) { star.magnitude = randFloat(1.5f, 4.0f); star.starType = 1; }
        else { star.magnitude = randFloat(4.0f, 6.5f); star.starType = 2; }

        if (star.starType == 1) star.temperature = randFloat(3000.0f, 5000.0f);
        else if (star.starType == 0) star.temperature = randFloat(4000.0f, 8000.0f);
        else star.temperature = randFloat(2500.0f, 4000.0f);

        star.twinkle = randFloat(0.5f, 3.0f);
        star.sizeFactor = randFloat(0.5f, 2.0f);
        star.size = magToSize(star.magnitude) * star.sizeFactor;
        star.textureIndex = (randFloat(0.0f, 1.0f) < 0.5f) ? 0 : 1;
    }

    void initStars() {
        for (int i = 0; i < NUM_STARS; ++i)
            initStar(stars[i], true);
    }

    void loadStarTexture(const std::string& path, VkImage& outTex,
                         VkDeviceMemory& outMem, VkImageView& outView) {
        SDL_Surface* img = png::LoadPNG(path.c_str());
        if (!img || !img->pixels)
            throw mx::Exception("Failed to load star texture: " + path);

        int w = img->w, h = img->h;
        VkDeviceSize imageSize = w * h * 4;

        
        VkBuffer stagingBuf;
        VkDeviceMemory stagingMem;
        VkBufferCreateInfo bufInfo{};
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size = imageSize;
        bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK_RESULT(vkCreateBuffer(device, &bufInfo, nullptr, &stagingBuf));

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device, stagingBuf, &memReq);
        VkMemoryAllocateInfo alloc{};
        alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc.allocationSize = memReq.size;
        alloc.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(device, &alloc, nullptr, &stagingMem));
        vkBindBufferMemory(device, stagingBuf, stagingMem, 0);

        void* data;
        vkMapMemory(device, stagingMem, 0, imageSize, 0, &data);
        memcpy(data, img->pixels, (size_t)imageSize);
        vkUnmapMemory(device, stagingMem);
        SDL_FreeSurface(img);

        
        VkImageCreateInfo imgInfo{};
        imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgInfo.imageType = VK_IMAGE_TYPE_2D;
        imgInfo.extent = { (uint32_t)w, (uint32_t)h, 1 };
        imgInfo.mipLevels = 1;
        imgInfo.arrayLayers = 1;
        imgInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK_RESULT(vkCreateImage(device, &imgInfo, nullptr, &outTex));

        vkGetImageMemoryRequirements(device, outTex, &memReq);
        alloc.allocationSize = memReq.size;
        alloc.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(device, &alloc, nullptr, &outMem));
        vkBindImageMemory(device, outTex, outMem, 0);

        transitionImageLayout(outTex, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuf, outTex, (uint32_t)w, (uint32_t)h);
        transitionImageLayout(outTex, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(device, stagingBuf, nullptr);
        vkFreeMemory(device, stagingMem, nullptr);

        
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = outTex;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &outView));

        
        if (sStarSampler == VK_NULL_HANDLE) {
            VkSamplerCreateInfo si{};
            si.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            si.magFilter = VK_FILTER_LINEAR;
            si.minFilter = VK_FILTER_LINEAR;
            si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            si.anisotropyEnable = VK_FALSE;
            si.maxAnisotropy = 1.0f;
            si.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            si.unnormalizedCoordinates = VK_FALSE;
            si.compareEnable = VK_FALSE;
            si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            VK_CHECK_RESULT(vkCreateSampler(device, &si, nullptr, &sStarSampler));
        }
    }

    void createStarDescriptorSetLayout() {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
        bindings[0] = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
        bindings[1] = { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr };

        VkDescriptorSetLayoutCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ci.bindingCount = (uint32_t)bindings.size();
        ci.pBindings = bindings.data();
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &ci, nullptr, &sDescSetLayout));
    }

    void createStarPipeline() {
        auto vertCode = mx::readFile(util.path + "/data/star_vert.spv");
        auto fragCode = mx::readFile(util.path + "/data/star_frag.spv");

        VkShaderModuleCreateInfo smci{};
        smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        smci.codeSize = vertCode.size();
        smci.pCode = reinterpret_cast<const uint32_t*>(vertCode.data());
        VkShaderModule vertMod;
        VK_CHECK_RESULT(vkCreateShaderModule(device, &smci, nullptr, &vertMod));

        smci.codeSize = fragCode.size();
        smci.pCode = reinterpret_cast<const uint32_t*>(fragCode.data());
        VkShaderModule fragMod;
        VK_CHECK_RESULT(vkCreateShaderModule(device, &smci, nullptr, &fragMod));

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertMod;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragMod;
        stages[1].pName = "main";

        VkVertexInputBindingDescription bind{};
        bind.binding = 0;
        bind.stride = sizeof(StarVertex);
        bind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 3> attrs{};
        attrs[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(StarVertex, pos) };
        attrs[1] = { 1, 0, VK_FORMAT_R32_SFLOAT, offsetof(StarVertex, size) };
        attrs[2] = { 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(StarVertex, color) };

        VkPipelineVertexInputStateCreateInfo vi{};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.vertexBindingDescriptionCount = 1;
        vi.pVertexBindingDescriptions = &bind;
        vi.vertexAttributeDescriptionCount = (uint32_t)attrs.size();
        vi.pVertexAttributeDescriptions = attrs.data();

        VkPipelineInputAssemblyStateCreateInfo ia{};
        ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        ia.primitiveRestartEnable = VK_FALSE;

        std::vector<VkDynamicState> dynStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dyn{};
        dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dyn.dynamicStateCount = (uint32_t)dynStates.size();
        dyn.pDynamicStates = dynStates.data();

        VkPipelineViewportStateCreateInfo vps{};
        vps.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vps.viewportCount = 1;
        vps.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rast{};
        rast.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rast.polygonMode = VK_POLYGON_MODE_FILL;
        rast.lineWidth = 1.0f;
        rast.cullMode = VK_CULL_MODE_NONE;
        rast.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo ms{};
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState cba{};
        cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        cba.blendEnable = VK_TRUE;
        cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        cba.colorBlendOp = VK_BLEND_OP_ADD;
        cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        cba.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo cb{};
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.attachmentCount = 1;
        cb.pAttachments = &cba;

        VkPipelineDepthStencilStateCreateInfo ds{};
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.depthTestEnable = VK_TRUE;
        ds.depthWriteEnable = VK_FALSE;  
        ds.depthCompareOp = VK_COMPARE_OP_LESS;

        VkPipelineLayoutCreateInfo plci{};
        plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        plci.setLayoutCount = 1;
        plci.pSetLayouts = &sDescSetLayout;
        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &plci, nullptr, &sPipelineLayout));

        VkGraphicsPipelineCreateInfo pi{};
        pi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pi.stageCount = 2;
        pi.pStages = stages;
        pi.pVertexInputState = &vi;
        pi.pInputAssemblyState = &ia;
        pi.pViewportState = &vps;
        pi.pRasterizationState = &rast;
        pi.pMultisampleState = &ms;
        pi.pDepthStencilState = &ds;
        pi.pColorBlendState = &cb;
        pi.pDynamicState = &dyn;
        pi.layout = sPipelineLayout;
        pi.renderPass = renderPass;
        pi.subpass = 0;
        VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pi, nullptr, &sStarPipeline));

        vkDestroyShaderModule(device, fragMod, nullptr);
        vkDestroyShaderModule(device, vertMod, nullptr);
    }

    void createStarVertexBuffer() {
        VkDeviceSize vbSize = sizeof(StarVertex) * NUM_STARS;
        VkBufferCreateInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bi.size = vbSize;
        bi.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK_RESULT(vkCreateBuffer(device, &bi, nullptr, &sVertexBuffer));

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device, sVertexBuffer, &memReq);
        VkMemoryAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = memReq.size;
        ai.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(device, &ai, nullptr, &sVertexBufferMemory));
        vkBindBufferMemory(device, sVertexBuffer, sVertexBufferMemory, 0);
        vkMapMemory(device, sVertexBufferMemory, 0, vbSize, 0, &sVertexBufferMapped);

        
        VkDeviceSize ibSize = sizeof(uint32_t) * NUM_STARS;
        bi.size = ibSize;
        bi.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        VK_CHECK_RESULT(vkCreateBuffer(device, &bi, nullptr, &sIndexBuffer));

        vkGetBufferMemoryRequirements(device, sIndexBuffer, &memReq);
        ai.allocationSize = memReq.size;
        ai.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(device, &ai, nullptr, &sIndexBufferMemory));
        vkBindBufferMemory(device, sIndexBuffer, sIndexBufferMemory, 0);
        vkMapMemory(device, sIndexBufferMemory, 0, ibSize, 0, &sIndexBufferMapped);
    }

    void createStarDescriptorPoolAndSets() {
        uint32_t imgCount = (uint32_t)swapChainImages.size();

        
        sUniformBuffers.resize(imgCount);
        sUniformMemory.resize(imgCount);
        sUniformMapped.resize(imgCount);
        for (uint32_t i = 0; i < imgCount; i++) {
            VkBufferCreateInfo bi{};
            bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bi.size = sizeof(mx::UniformBufferObject);
            bi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VK_CHECK_RESULT(vkCreateBuffer(device, &bi, nullptr, &sUniformBuffers[i]));

            VkMemoryRequirements memReq;
            vkGetBufferMemoryRequirements(device, sUniformBuffers[i], &memReq);
            VkMemoryAllocateInfo ai{};
            ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            ai.allocationSize = memReq.size;
            ai.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            VK_CHECK_RESULT(vkAllocateMemory(device, &ai, nullptr, &sUniformMemory[i]));
            vkBindBufferMemory(device, sUniformBuffers[i], sUniformMemory[i], 0);
            vkMapMemory(device, sUniformMemory[i], 0, sizeof(mx::UniformBufferObject), 0, &sUniformMapped[i]);
        }

        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imgCount * 2 };
        poolSizes[1] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, imgCount * 2 };

        VkDescriptorPoolCreateInfo pi{};
        pi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pi.poolSizeCount = (uint32_t)poolSizes.size();
        pi.pPoolSizes = poolSizes.data();
        pi.maxSets = imgCount * 2;
        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pi, nullptr, &sDescPool));

        
        std::vector<VkDescriptorSetLayout> layouts(imgCount * 2, sDescSetLayout);
        VkDescriptorSetAllocateInfo dai{};
        dai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dai.descriptorPool = sDescPool;
        dai.descriptorSetCount = imgCount * 2;
        dai.pSetLayouts = layouts.data();

        std::vector<VkDescriptorSet> allSets(imgCount * 2);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &dai, allSets.data()));

        sDescSets1.resize(imgCount);
        sDescSets2.resize(imgCount);
        for (uint32_t i = 0; i < imgCount; i++) {
            sDescSets1[i] = allSets[i];
            sDescSets2[i] = allSets[imgCount + i];
        }

        for (size_t i = 0; i < imgCount; i++) {
            VkDescriptorBufferInfo bufInfo{};
            bufInfo.buffer = sUniformBuffers[i];  
            bufInfo.offset = 0;
            bufInfo.range = sizeof(mx::UniformBufferObject);

            
            VkDescriptorImageInfo imgInfo1{};
            imgInfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imgInfo1.imageView = sTexView1;
            imgInfo1.sampler = sStarSampler;

            std::array<VkWriteDescriptorSet, 2> writes{};
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = sDescSets1[i];
            writes[0].dstBinding = 0;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[0].descriptorCount = 1;
            writes[0].pImageInfo = &imgInfo1;

            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = sDescSets1[i];
            writes[1].dstBinding = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[1].descriptorCount = 1;
            writes[1].pBufferInfo = &bufInfo;

            vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);

            
            VkDescriptorImageInfo imgInfo2{};
            imgInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imgInfo2.imageView = sTexView2;
            imgInfo2.sampler = sStarSampler;

            writes[0].dstSet = sDescSets2[i];
            writes[0].pImageInfo = &imgInfo2;
            writes[1].dstSet = sDescSets2[i];

            vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
        }
    }

    void initStarResources() {
        std::string dp = util.path;
        loadStarTexture(dp + "/data/star.png",  sTex1, sTexMem1, sTexView1);
        loadStarTexture(dp + "/data/star2.png", sTex2, sTexMem2, sTexView2);
        createStarDescriptorSetLayout();
        createStarPipeline();
        createStarVertexBuffer();
        createStarDescriptorPoolAndSets();
        sLastUpdateTime = SDL_GetTicks();
        starsInitialized = true;
        SDL_Log("Star system initialized with %d stars", NUM_STARS);
    }

    void updateStars(float deltaTime) {
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        float time = SDL_GetTicks() * 0.001f;

        sTex1Indices.clear();
        sTex2Indices.clear();
        sTex1Indices.reserve(NUM_STARS / 2);
        sTex2Indices.reserve(NUM_STARS / 2);

        for (int i = 0; i < NUM_STARS; ++i) {
            auto& s = stars[i];

            
            s.x += s.vx * deltaTime;
            s.y += s.vy * deltaTime;
            s.z += s.vz * deltaTime;

            
            if (starExclusionRadius > 0.0f) {
                float d2 = s.x*s.x + s.y*s.y + s.z*s.z;
                if (d2 <= starExclusionRadius * starExclusionRadius) {
                    initStar(s, false);
                    s.z = randFloat(-400.0f, -150.0f);
                    continue;
                }
            }

            
            if (s.z > 10.0f) {
                initStar(s, false);
                s.z = randFloat(-400.0f, -150.0f);
                continue;
            }

            if (!s.active) continue;

            starVertices[i].pos[0] = s.x;
            starVertices[i].pos[1] = s.y;
            starVertices[i].pos[2] = s.z;

            float twinkle = 0.7f + 0.3f * sin(time * s.twinkle);
            float sz = s.size * twinkle;
            float dist = glm::length(glm::vec3(s.x, s.y, s.z));
            float perspScale = glm::clamp(150.0f / glm::max(dist, 1.0f), 0.05f, 20.0f);
            sz *= perspScale;
            starVertices[i].size = sz;

            glm::vec3 c = starColor(s.temperature);
            float alpha = magToAlpha(s.magnitude) * twinkle;
            starVertices[i].color[0] = c.r;
            starVertices[i].color[1] = c.g;
            starVertices[i].color[2] = c.b;
            starVertices[i].color[3] = alpha;

            if (s.textureIndex == 0) sTex1Indices.push_back((uint32_t)i);
            else sTex2Indices.push_back((uint32_t)i);
        }

        if (sVertexBufferMapped)
            memcpy(sVertexBufferMapped, starVertices, sizeof(starVertices));
    }

    void drawStars(VkCommandBuffer cmd, uint32_t imageIndex,
                   const glm::mat4& viewMat, const glm::mat4& projMat, float time) {
        if (!starsInitialized) return;

        Uint32 now = SDL_GetTicks();
        float dt = (now - sLastUpdateTime) / 1000.0f;
        sLastUpdateTime = now;
        updateStars(dt);

        
        mx::UniformBufferObject starUbo{};
        starUbo.model = glm::mat4(1.0f);
        starUbo.view = viewMat;
        starUbo.proj = projMat;
        starUbo.color = glm::vec4(1.0f);
        starUbo.params = glm::vec4(time, 0, 0, 0);
        memcpy(sUniformMapped[imageIndex], &starUbo, sizeof(starUbo));

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sStarPipeline);

        VkBuffer vb[] = { sVertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, vb, offsets);
        vkCmdBindIndexBuffer(cmd, sIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        
        if (!sTex1Indices.empty() && sIndexBufferMapped) {
            memcpy(sIndexBufferMapped, sTex1Indices.data(), sTex1Indices.size() * sizeof(uint32_t));
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                sPipelineLayout, 0, 1, &sDescSets1[imageIndex], 0, nullptr);
            vkCmdDrawIndexed(cmd, (uint32_t)sTex1Indices.size(), 1, 0, 0, 0);
        }

        
        if (!sTex2Indices.empty() && sIndexBufferMapped) {
            memcpy(sIndexBufferMapped, sTex2Indices.data(), sTex2Indices.size() * sizeof(uint32_t));
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                sPipelineLayout, 0, 1, &sDescSets2[imageIndex], 0, nullptr);
            vkCmdDrawIndexed(cmd, (uint32_t)sTex2Indices.size(), 1, 0, 0, 0);
        }
    }

    void cleanupStars() {
        if (!starsInitialized) return;
        vkDeviceWaitIdle(device);

        if (sStarPipeline != VK_NULL_HANDLE) { vkDestroyPipeline(device, sStarPipeline, nullptr); sStarPipeline = VK_NULL_HANDLE; }
        if (sPipelineLayout != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device, sPipelineLayout, nullptr); sPipelineLayout = VK_NULL_HANDLE; }
        if (sDescPool != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device, sDescPool, nullptr); sDescPool = VK_NULL_HANDLE; }
        if (sDescSetLayout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device, sDescSetLayout, nullptr); sDescSetLayout = VK_NULL_HANDLE; }
        if (sVertexBuffer != VK_NULL_HANDLE) { vkDestroyBuffer(device, sVertexBuffer, nullptr); vkFreeMemory(device, sVertexBufferMemory, nullptr); sVertexBuffer = VK_NULL_HANDLE; }
        if (sIndexBuffer != VK_NULL_HANDLE) { vkDestroyBuffer(device, sIndexBuffer, nullptr); vkFreeMemory(device, sIndexBufferMemory, nullptr); sIndexBuffer = VK_NULL_HANDLE; }
        for (size_t i = 0; i < sUniformBuffers.size(); i++) {
            if (sUniformBuffers[i] != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, sUniformBuffers[i], nullptr);
                vkFreeMemory(device, sUniformMemory[i], nullptr);
            }
        }
        sUniformBuffers.clear(); sUniformMemory.clear(); sUniformMapped.clear();
        if (sStarSampler != VK_NULL_HANDLE) { vkDestroySampler(device, sStarSampler, nullptr); sStarSampler = VK_NULL_HANDLE; }
        if (sTexView1 != VK_NULL_HANDLE) { vkDestroyImageView(device, sTexView1, nullptr); sTexView1 = VK_NULL_HANDLE; }
        if (sTex1 != VK_NULL_HANDLE) { vkDestroyImage(device, sTex1, nullptr); vkFreeMemory(device, sTexMem1, nullptr); sTex1 = VK_NULL_HANDLE; }
        if (sTexView2 != VK_NULL_HANDLE) { vkDestroyImageView(device, sTexView2, nullptr); sTexView2 = VK_NULL_HANDLE; }
        if (sTex2 != VK_NULL_HANDLE) { vkDestroyImage(device, sTex2, nullptr); vkFreeMemory(device, sTexMem2, nullptr); sTex2 = VK_NULL_HANDLE; }

        starsInitialized = false;
    }

    void event(SDL_Event& e) override {
        if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
            quit();
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
            effectIndex = (effectIndex + 1) % 10;  // 0=off, 1=kaleidoscope, 2=ripple/twist, 3=rotate/warp, 4=spiral, 5=gravity/spiral, 6=rotating/zoom, 7=chromatic/barrel, 8=bend/warp, 9=bubble/distort
            std::cout << "index: " << effectIndex << "\n";
        }
        
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            dragging = true;
            lastMouseX = e.button.x;
            lastMouseY = e.button.y;
        } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
            dragging = false;
        } else if (e.type == SDL_MOUSEMOTION && dragging) {
            int dx = e.motion.x - lastMouseX;
            int dy = e.motion.y - lastMouseY;
            rotationY += dx * 0.5f;
            rotationX += dy * 0.5f;
            lastMouseX = e.motion.x;
            lastMouseY = e.motion.y;
        }
        
        if (e.type == SDL_MOUSEWHEEL) {
            cameraDistance -= e.wheel.y * 0.5f;
            if (cameraDistance < 1.0f) cameraDistance = 1.0f;
            if (cameraDistance > 20.0f) cameraDistance = 20.0f;
        }

        if (e.type == SDL_CONTROLLERDEVICEADDED) {
            if (!controller) {
                controller = SDL_GameControllerOpen(e.cdevice.which);
                if (controller)
                    SDL_Log("Controller connected: %s", SDL_GameControllerName(controller));
            }
        }
        if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
            if (controller && e.cdevice.which == SDL_JoystickInstanceID(
                    SDL_GameControllerGetJoystick(controller))) {
                SDL_GameControllerClose(controller);
                controller = nullptr;
                SDL_Log("Controller disconnected");
            }
        }

        if (e.type == SDL_CONTROLLERBUTTONDOWN) {
            switch (e.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_A:
                    effectIndex = (effectIndex + 1) % 10;
                    break;
                case SDL_CONTROLLER_BUTTON_B:
                    effectIndex = (effectIndex - 1 + 10) % 10;
                    break;
                case SDL_CONTROLLER_BUTTON_START:
                    quit();
                    break;
                default:
                    break;
            }
        }
    }

    void cleanup() override {
        if (controller) {
            SDL_GameControllerClose(controller);
            controller = nullptr;
        }
        cleanupStars();
        model.cleanup(device);
        mx::VKWindow::cleanup();
    }

private:
    cv::VideoCapture cap;
    mx::MXModel model;
    int camera_width = 0;
    int camera_height = 0;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    bool dragging = false;
    int lastMouseX = 0;
    int lastMouseY = 0;
    int effectIndex = 0;  // 0=off, 1=kaleidoscope, 2=ripple/twist, 3=rotate/warp, 4=spiral, 5=gravity/spiral, 6=rotating/zoom, 7=chromatic/barrel, 8=bend/warp, 9=bubble/distort

    
    SDL_GameController* controller = nullptr;
    static constexpr float STICK_DEADZONE = 8000.0f;
    static constexpr float STICK_SENSITIVITY = 150.0f;
    static constexpr float ZOOM_SENSITIVITY = 5.0f;

    
    bool starsInitialized = false;
    float starExclusionRadius = 1.5f;  
    Uint32 sLastUpdateTime = 0;

    Star stars[NUM_STARS];
    StarVertex starVertices[NUM_STARS];
    std::vector<uint32_t> sTex1Indices;
    std::vector<uint32_t> sTex2Indices;

    VkImage sTex1 = VK_NULL_HANDLE;
    VkDeviceMemory sTexMem1 = VK_NULL_HANDLE;
    VkImageView sTexView1 = VK_NULL_HANDLE;
    VkImage sTex2 = VK_NULL_HANDLE;
    VkDeviceMemory sTexMem2 = VK_NULL_HANDLE;
    VkImageView sTexView2 = VK_NULL_HANDLE;
    VkSampler sStarSampler = VK_NULL_HANDLE;

    VkDescriptorSetLayout sDescSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool sDescPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> sDescSets1;
    std::vector<VkDescriptorSet> sDescSets2;

    VkPipelineLayout sPipelineLayout = VK_NULL_HANDLE;
    VkPipeline sStarPipeline = VK_NULL_HANDLE;

    VkBuffer sVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory sVertexBufferMemory = VK_NULL_HANDLE;
    void* sVertexBufferMapped = nullptr;
    VkBuffer sIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory sIndexBufferMemory = VK_NULL_HANDLE;
    void* sIndexBufferMapped = nullptr;

    std::vector<VkBuffer> sUniformBuffers;
    std::vector<VkDeviceMemory> sUniformMemory;
    std::vector<void*> sUniformMapped;
    std::string filename;
};

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
        Moon window(args.path, args.width, args.height, args.fullscreen);
        window.initVulkan();
        if(args.filename.empty())
            window.openFile(args.path + "/data/eye1.mp4");
        else if(args.filename == "camera") 
            window.openCamera(0, args.width, args.height);
        else 
            window.openFile(args.filename);
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
        return EXIT_FAILURE;
    } catch(std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return EXIT_SUCCESS;
}
