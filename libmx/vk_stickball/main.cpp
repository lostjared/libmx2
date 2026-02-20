

#include "vk.hpp"
#include "vk_model.hpp"
#include "loadpng.hpp"
#include "SDL.h"
#include "argz.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <cmath>
#include <string>
#include <algorithm>
#include <array>
#include <cstring>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


static constexpr float TABLE_W      = 12.0f;
static constexpr float TABLE_H      = 6.0f;
static constexpr float TABLE_HALF_W = TABLE_W / 2.0f;
static constexpr float TABLE_HALF_H = TABLE_H / 2.0f;
static constexpr float BUMPER_W     = 0.3f;
static constexpr float POCKET_R     = 0.45f;
static constexpr float BALL_RADIUS  = 0.18f;
static constexpr float CUE_LENGTH   = 2.5f;
static constexpr int   NUM_BALLS    = 16;
static constexpr float FRICTION     = 0.9988f;
static constexpr float MIN_SPEED    = 0.001f;
static constexpr float MAX_POWER    = 4.0f;
static constexpr int   MAX_DRAWS    = 48;   



static constexpr float PKT_INS = 0.15f; 
static const glm::vec2 POCKETS[6] = {
    { -TABLE_HALF_W + PKT_INS,  TABLE_HALF_H - PKT_INS },   
    {  0.0f,                    TABLE_HALF_H - 0.05f   },   
    {  TABLE_HALF_W - PKT_INS,  TABLE_HALF_H - PKT_INS },   
    { -TABLE_HALF_W + PKT_INS, -TABLE_HALF_H + PKT_INS },   
    {  0.0f,                   -TABLE_HALF_H + 0.05f   },   
    {  TABLE_HALF_W - PKT_INS, -TABLE_HALF_H + PKT_INS },   
};


static const glm::vec3 BALL_COLORS[NUM_BALLS] = {
    {1.0f, 1.0f, 1.0f},   
    {1.0f, 0.84f, 0.0f},  
    {0.0f, 0.0f, 0.8f},   
    {0.9f, 0.0f, 0.0f},   
    {0.5f, 0.0f, 0.5f},   
    {1.0f, 0.5f, 0.0f},   
    {0.0f, 0.5f, 0.0f},   
    {0.55f, 0.0f, 0.0f},  
    {0.1f, 0.1f, 0.1f},   
    {1.0f, 0.84f, 0.0f},  
    {0.0f, 0.0f, 0.8f},   
    {0.9f, 0.0f, 0.0f},   
    {0.5f, 0.0f, 0.5f},   
    {1.0f, 0.5f, 0.0f},   
    {0.0f, 0.5f, 0.0f},   
    {0.55f, 0.0f, 0.0f},  
};


static float randFloat(float mn, float mx) {
    static std::random_device rd;
    static std::default_random_engine eng(rd());
    std::uniform_real_distribution<float> d(mn, mx);
    return d(eng);
}


struct PoolBall {
    glm::vec2 pos{0.0f};
    glm::vec2 vel{0.0f};
    bool active    = true;
    bool pocketed  = false;
    int  number    = 0;
    float spinAngle = 0.0f;

    bool isMoving() const { return glm::length(vel) > MIN_SPEED; }
};


enum class GamePhase { Aiming, Charging, Rolling, Placing, GameOver };




class PoolWindow : public mx::VKWindow {
public:
    PoolWindow(const std::string &path, int wx, int wy, bool full)
        : mx::VKWindow("-[ 3D Pool / Vulkan ]-", wx, wy, full) {
        setPath(path);
    }
    ~PoolWindow() override = default;

    
    void initVulkan() override {
        mx::VKWindow::initVulkan();

        
        
        {
            uint32_t white = 0xFFFFFFFF;
            VkBuffer stg; VkDeviceMemory stgMem;
            createBuffer(4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         stg, stgMem);
            void *d; vkMapMemory(device, stgMem, 0, 4, 0, &d);
            memcpy(d, &white, 4);
            vkUnmapMemory(device, stgMem);
            copyBufferToImage(stg, textureImage, 1, 1);
            transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            vkDestroyBuffer(device, stg, nullptr);
            vkFreeMemory(device, stgMem, nullptr);
        }

        
        loadTableTexture();

        
        createObjectPool();

        sphereModel.reset(new mx::MXModel());
        sphereModel->load(util.path + "/data/geosphere.mxmod.z", 1.0f);
        sphereModel->upload(device, physicalDevice, commandPool, graphicsQueue);

        cylinderModel.reset(new mx::MXModel());
        cylinderModel->load(util.path + "/data/pocket.mxmod.z", 0.285714f);
        cylinderModel->upload(device, physicalDevice, commandPool, graphicsQueue);

        cubeModel.reset(new mx::MXModel());
        cubeModel->load(util.path + "/data/cube.mxmod.z", 2.0f);
        cubeModel->upload(device, physicalDevice, commandPool, graphicsQueue);

        backgroundSprite = createSprite(util.path + "/data/background.png", "", "");

        resetGame();
    }

    
    void proc() override {
        float now = SDL_GetTicks() / 1000.0f;
        float dt  = now - lastTime;
        if (dt > 0.05f) dt = 0.05f;
        lastTime = now;

        switch (phase) {
        case GamePhase::Aiming:   handleAiming(dt);   break;
        case GamePhase::Charging: handleCharging(dt);  break;
        case GamePhase::Rolling:
            updatePhysics(dt);
            if (!anyBallMoving()) {
                if (balls[0].pocketed) {
                    phase = GamePhase::Placing;
                    balls[0].active   = true;
                    balls[0].pocketed = false;
                    balls[0].pos      = glm::vec2(-TABLE_HALF_W * 0.5f, 0.0f);
                    balls[0].vel      = glm::vec2(0.0f);
                } else {
                    phase = GamePhase::Aiming;
                }
                checkGameOver();
            }
            break;
        case GamePhase::Placing:  handlePlacing(dt);   break;
        case GamePhase::GameOver: break;
        }

        
        for (auto &b : balls) {
            if (b.active && b.isMoving()) {
                b.spinAngle += glm::length(b.vel) * 5.0f * dt;
            }
        }

        
        {
            const Uint8 *k = SDL_GetKeyboardState(nullptr);
            if (k[SDL_SCANCODE_A]) camAngle -= 1.2f * dt;
            if (k[SDL_SCANCODE_S]) camAngle += 1.2f * dt;
            if (k[SDL_SCANCODE_W]) camZoom -= 5.0f * dt;
            if (k[SDL_SCANCODE_E]) camZoom += 5.0f * dt;
            camZoom = glm::clamp(camZoom, 5.0f, 25.0f);
        }

        
        printText("-[ 3D Pool ]-", 15, 15, {255, 255, 255, 255});
        printText("Shots: " + std::to_string(shotCount), 15, 45, {255, 255, 0, 255});
        int rem = 0;
        for (int i = 1; i < NUM_BALLS; i++)
            if (balls[i].active && !balls[i].pocketed) rem++;
        printText("Balls: " + std::to_string(rem), 15, 75, {200, 200, 200, 255});

        if (phase == GamePhase::Aiming)
            printText("Arrow Keys: Aim | Space: Charge & Shoot", 15, h - 40, {180, 180, 180, 255});
        else if (phase == GamePhase::Charging) {
            int pct = static_cast<int>(chargeAmount / MAX_POWER * 100.0f);
            printText("Power: " + std::to_string(pct) + "%", 15, 105,
                      {255, static_cast<Uint8>(255 - pct * 2), 0, 255});
        } else if (phase == GamePhase::Placing)
            printText("Arrows: move cue ball | Enter: place", 15, h - 40, {255, 100, 100, 255});
        else if (phase == GamePhase::GameOver) {
            printText("Game Over", w / 2 - 70, h / 2 - 30, {255, 50, 50, 255});
            printText("Press Enter to play again", w / 2 - 120, h / 2 + 10, {200, 200, 200, 255});
        }
    }

    
    void draw() override {
        uint32_t imageIndex = 0;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                                 imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) { recreateSwapChain(); return; }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw mx::Exception("Failed to acquire swap chain image!");

        VK_CHECK_RESULT(vkResetCommandBuffer(commandBuffers[imageIndex], 0));
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
            throw mx::Exception("Failed to begin recording command buffer!");

        VkRenderPassBeginInfo rpInfo{};
        rpInfo.sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass         = renderPass;
        rpInfo.framebuffer        = swapChainFramebuffers[imageIndex];
        rpInfo.renderArea.offset  = {0, 0};
        rpInfo.renderArea.extent  = swapChainExtent;
        std::array<VkClearValue, 2> cv{};
        cv[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
        cv[1].depthStencil = {1.0f, 0};
        rpInfo.clearValueCount = static_cast<uint32_t>(cv.size());
        rpInfo.pClearValues    = cv.data();

        VkCommandBuffer cmd = commandBuffers[imageIndex];
        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport vp{}; vp.width = (float)swapChainExtent.width;
        vp.height = (float)swapChainExtent.height; vp.maxDepth = 1.0f;
        VkRect2D sc{}; sc.extent = swapChainExtent;
        vkCmdSetViewport(cmd, 0, 1, &vp);
        vkCmdSetScissor(cmd, 0, 1, &sc);

        // Draw background image first so it stays fixed behind all 3D geometry
        if (spritePipeline != VK_NULL_HANDLE && backgroundSprite != nullptr) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline);
            backgroundSprite->drawSpriteRect(0, 0,
                static_cast<int>(swapChainExtent.width),
                static_cast<int>(swapChainExtent.height));
            backgroundSprite->renderSprites(cmd, spritePipelineLayout,
                swapChainExtent.width, swapChainExtent.height);
            backgroundSprite->clearQueue();
        }

        float aspect = vp.width / vp.height;
        float time   = SDL_GetTicks() / 1000.0f;

        
        float camDist   = camZoom;
        float camHeight = camZoom * 0.92f;
        glm::vec3 camPos    = glm::vec3(sinf(camAngle) * camDist, camHeight, cosf(camAngle) * camDist);
        glm::vec3 camTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::mat4 viewMat   = glm::lookAt(camPos, camTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projMat   = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        projMat[1][1] *= -1;

        VkPipeline pipe = useWireFrame ? graphicsPipelineMatrix : graphicsPipeline;
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);

        
        objDrawIndex = 0;

        
        drawModelTextured(cmd, imageIndex, cubeModel.get(), viewMat, projMat, time,
                  glm::vec3(0.0f, -0.15f, 0.0f),
                  glm::vec3(TABLE_HALF_W, 0.05f, TABLE_HALF_H),
                  glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        
        glm::vec4 wood(0.4f, 0.22f, 0.05f, 1.0f);
        drawModel(cmd, imageIndex, cubeModel.get(), viewMat, projMat, time,
                  glm::vec3(0, 0, -TABLE_HALF_H - BUMPER_W/2),
                  glm::vec3(TABLE_HALF_W + BUMPER_W, 0.15f, BUMPER_W/2), wood);
        drawModel(cmd, imageIndex, cubeModel.get(), viewMat, projMat, time,
                  glm::vec3(0, 0, TABLE_HALF_H + BUMPER_W/2),
                  glm::vec3(TABLE_HALF_W + BUMPER_W, 0.15f, BUMPER_W/2), wood);
        drawModel(cmd, imageIndex, cubeModel.get(), viewMat, projMat, time,
                  glm::vec3(-TABLE_HALF_W - BUMPER_W/2, 0, 0),
                  glm::vec3(BUMPER_W/2, 0.15f, TABLE_HALF_H + BUMPER_W), wood);
        drawModel(cmd, imageIndex, cubeModel.get(), viewMat, projMat, time,
                  glm::vec3(TABLE_HALF_W + BUMPER_W/2, 0, 0),
                  glm::vec3(BUMPER_W/2, 0.15f, TABLE_HALF_H + BUMPER_W), wood);

  
        for (int i = 0; i < 6; i++) {
            drawModel(cmd, imageIndex, cylinderModel.get(), viewMat, projMat, time,
                      glm::vec3(POCKETS[i].x, -0.098f, POCKETS[i].y),
                      glm::vec3(POCKET_R, 0.001f, POCKET_R),
                      glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        }

        for (int i = 0; i < NUM_BALLS; i++) {
            if (!balls[i].active || balls[i].pocketed) continue;
            glm::mat4 m(1.0f);
            m = glm::translate(m, glm::vec3(balls[i].pos.x, BALL_RADIUS, balls[i].pos.y));
            m = glm::rotate(m, balls[i].spinAngle, glm::vec3(0.0f, 1.0f, 0.0f));
            m = glm::scale(m, glm::vec3(BALL_RADIUS));
            drawModelMat(cmd, imageIndex, sphereModel.get(), viewMat, projMat, time,
                         m, glm::vec4(BALL_COLORS[i], 1.0f));
        }

        
        if ((phase == GamePhase::Aiming || phase == GamePhase::Charging) && balls[0].active) {
            float offset   = (phase == GamePhase::Charging)
                             ? (chargeAmount / MAX_POWER * 1.0f) : 0.0f;
            float stickDist = BALL_RADIUS + 0.3f + offset;
            glm::vec2 dir(cosf(cueAngle), sinf(cueAngle));
            glm::vec2 sc2 = balls[0].pos - dir * (stickDist + CUE_LENGTH * 0.5f);

            glm::mat4 m(1.0f);
            m = glm::translate(m, glm::vec3(sc2.x, BALL_RADIUS + 0.05f, sc2.y));
            m = glm::rotate(m, -cueAngle, glm::vec3(0.0f, 1.0f, 0.0f));
            m = glm::scale(m, glm::vec3(CUE_LENGTH * 0.5f, 0.03f, 0.03f));
            float pct = chargeAmount / MAX_POWER;
            glm::vec4 cueCol = (phase == GamePhase::Charging)
                ? glm::vec4(0.55f + pct*0.45f, 0.27f*(1-pct), 0.07f*(1-pct), 1)
                : glm::vec4(0.55f, 0.27f, 0.07f, 1.0f);
            drawModelMat(cmd, imageIndex, cubeModel.get(), viewMat, projMat, time, m, cueCol);

            
            float aimLen = 2.0f;
            glm::vec2 ac = balls[0].pos + dir * (BALL_RADIUS + aimLen * 0.5f);
            glm::mat4 am(1.0f);
            am = glm::translate(am, glm::vec3(ac.x, BALL_RADIUS + 0.02f, ac.y));
            am = glm::rotate(am, -cueAngle, glm::vec3(0.0f, 1.0f, 0.0f));
            am = glm::scale(am, glm::vec3(aimLen * 0.5f, 0.005f, 0.005f));
            drawModelMat(cmd, imageIndex, cubeModel.get(), viewMat, projMat, time,
                         am, glm::vec4(1, 1, 0, 0.5f));
        }

        
        if (spritePipeline != VK_NULL_HANDLE && !sprites.empty()) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline);
            for (auto &sp : sprites) {
                if (!sp) continue;
                sp->renderSprites(cmd, spritePipelineLayout,
                                  swapChainExtent.width, swapChainExtent.height);
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

        VkSubmitInfo si{}; si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore wait[] = { imageAvailableSemaphore };
        VkPipelineStageFlags ws[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        si.waitSemaphoreCount = 1; si.pWaitSemaphores = wait; si.pWaitDstStageMask = ws;
        si.commandBufferCount = 1; si.pCommandBuffers = &cmd;
        VkSemaphore sig[] = { renderFinishedSemaphore };
        si.signalSemaphoreCount = 1; si.pSignalSemaphores = sig;
        if (vkQueueSubmit(graphicsQueue, 1, &si, VK_NULL_HANDLE) != VK_SUCCESS)
            throw mx::Exception("Failed to submit draw command buffer!");

        VkPresentInfoKHR pi{}; pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        pi.waitSemaphoreCount = 1; pi.pWaitSemaphores = sig;
        pi.swapchainCount = 1; pi.pSwapchains = &swapChain; pi.pImageIndices = &imageIndex;
        result = vkQueuePresentKHR(presentQueue, &pi);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
            recreateSwapChain();
        else if (result != VK_SUCCESS)
            throw mx::Exception("Failed to present swap chain image!");

        vkQueueWaitIdle(graphicsQueue);
        clearTextQueue();
        for (auto &sp : sprites) { if (sp) sp->clearQueue(); }
    }

    
    void event(SDL_Event &e) override {
        if (e.type == SDL_QUIT ||
            (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
            quit(); return;
        }
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
            case SDLK_r: resetGame(); break;
            case SDLK_p: setWireFrame(!getWireFrame()); break;
            case SDLK_SPACE:
                if (phase == GamePhase::Aiming) {
                    phase = GamePhase::Charging;
                    chargeAmount = 0.0f;
                }
                break;
            case SDLK_RETURN:
                if (phase == GamePhase::Placing) {
                    bool ok = true;
                    for (int i = 1; i < NUM_BALLS; i++) {
                        if (!balls[i].active || balls[i].pocketed) continue;
                        if (glm::length(balls[0].pos - balls[i].pos) < BALL_RADIUS * 2.5f)
                        { ok = false; break; }
                    }
                    if (ok) phase = GamePhase::Aiming;
                } else if (phase == GamePhase::GameOver) {
                    resetGame();
                }
                break;
            default: break;
            }
        }
        if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_SPACE
            && phase == GamePhase::Charging) {
            glm::vec2 dir(cosf(cueAngle), sinf(cueAngle));
            balls[0].vel = dir * chargeAmount;
            phase = GamePhase::Rolling;
            shotCount++;
            chargeAmount = 0.0f;
        }
    }

    
    void cleanup() override {
        if (sphereModel)   sphereModel->cleanup(device);
        if (cylinderModel) cylinderModel->cleanup(device);
        if (cubeModel)     cubeModel->cleanup(device);
        cleanupTableTexture();
        cleanupObjectPool();
        mx::VKWindow::cleanup();
    }

private:
    std::unique_ptr<mx::MXModel> sphereModel;
    std::unique_ptr<mx::MXModel> cylinderModel;
    std::unique_ptr<mx::MXModel> cubeModel;

    PoolBall  balls[NUM_BALLS];
    GamePhase phase        = GamePhase::Aiming;
    float     cueAngle     = 0.0f;
    float     chargeAmount = 0.0f;
    int       shotCount    = 0;
    float     lastTime     = 0.0f;
    float     camAngle     = 0.4f;   
    float     camZoom      = 13.0f;  

    
    VkImage        tableTexImage     = VK_NULL_HANDLE;
    VkDeviceMemory tableTexMemory    = VK_NULL_HANDLE;
    VkImageView    tableTexImageView = VK_NULL_HANDLE;

    mx::VKSprite  *backgroundSprite  = nullptr;
    

    void loadTableTexture() {
        std::string path = util.path + "/data/table.png";
        SDL_Surface *surface = png::LoadPNG(path.c_str());
        if (!surface) {
            SDL_Log("Failed to load table.png");
            return;
        }
        
        SDL_Surface *rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ABGR8888, 0);
        SDL_FreeSurface(surface);
        if (!rgba) {
            SDL_Log("Failed to convert table.png to RGBA");
            return;
        }
        VkDeviceSize imgSize = rgba->w * rgba->h * 4;
        
        VkBuffer stg; VkDeviceMemory stgMem;
        createBuffer(imgSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stg, stgMem);
        void *d;
        vkMapMemory(device, stgMem, 0, imgSize, 0, &d);
        memcpy(d, rgba->pixels, imgSize);
        vkUnmapMemory(device, stgMem);
        
        createImage(rgba->w, rgba->h, VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    tableTexImage, tableTexMemory);
        
        transitionImageLayout(tableTexImage, VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stg, tableTexImage, rgba->w, rgba->h);
        transitionImageLayout(tableTexImage, VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        vkDestroyBuffer(device, stg, nullptr);
        vkFreeMemory(device, stgMem, nullptr);
        SDL_FreeSurface(rgba);
        
        tableTexImageView = createImageView(tableTexImage, VK_FORMAT_R8G8B8A8_UNORM,
                                            VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void cleanupTableTexture() {
        if (tableTexImageView != VK_NULL_HANDLE)
            vkDestroyImageView(device, tableTexImageView, nullptr);
        if (tableTexImage != VK_NULL_HANDLE)
            vkDestroyImage(device, tableTexImage, nullptr);
        if (tableTexMemory != VK_NULL_HANDLE)
            vkFreeMemory(device, tableTexMemory, nullptr);
    }

    
    
    
    size_t                         swapCount = 0;
    VkDescriptorPool               objPool   = VK_NULL_HANDLE;
    std::vector<VkBuffer>          objUBOs;
    std::vector<VkDeviceMemory>    objUBOMem;
    std::vector<void*>             objUBOMapped;
    std::vector<VkDescriptorSet>   objDescSets;
    int                            objDrawIndex = 0;

    void createObjectPool() {
        swapCount = swapChainImages.size();
        size_t total = MAX_DRAWS * swapCount;

        
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(total);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(total);

        VkDescriptorPoolCreateInfo pi{};
        pi.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pi.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        pi.pPoolSizes    = poolSizes.data();
        pi.maxSets       = static_cast<uint32_t>(total);
        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pi, nullptr, &objPool));

        
        VkDeviceSize uboSize = sizeof(mx::UniformBufferObject);
        objUBOs.resize(total);
        objUBOMem.resize(total);
        objUBOMapped.resize(total);
        for (size_t i = 0; i < total; i++) {
            createBuffer(uboSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         objUBOs[i], objUBOMem[i]);
            VK_CHECK_RESULT(vkMapMemory(device, objUBOMem[i], 0, uboSize, 0, &objUBOMapped[i]));
        }

        
        std::vector<VkDescriptorSetLayout> layouts(total, descriptorSetLayout);
        VkDescriptorSetAllocateInfo ai{};
        ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool     = objPool;
        ai.descriptorSetCount = static_cast<uint32_t>(total);
        ai.pSetLayouts        = layouts.data();
        objDescSets.resize(total);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &ai, objDescSets.data()));

        
        for (size_t i = 0; i < total; i++) {
            VkDescriptorImageInfo imgInfo{};
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imgInfo.imageView   = textureImageView;
            imgInfo.sampler     = textureSampler;

            VkDescriptorBufferInfo bufInfo{};
            bufInfo.buffer = objUBOs[i];
            bufInfo.offset = 0;
            bufInfo.range  = uboSize;

            std::array<VkWriteDescriptorSet, 2> writes{};
            writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet          = objDescSets[i];
            writes[0].dstBinding      = 0;
            writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[0].descriptorCount = 1;
            writes[0].pImageInfo      = &imgInfo;

            writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet          = objDescSets[i];
            writes[1].dstBinding      = 1;
            writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[1].descriptorCount = 1;
            writes[1].pBufferInfo     = &bufInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()),
                                   writes.data(), 0, nullptr);
        }
    }

    void cleanupObjectPool() {
        for (size_t i = 0; i < objUBOs.size(); i++) {
            if (objUBOMem[i] != VK_NULL_HANDLE)
                vkUnmapMemory(device, objUBOMem[i]);
            if (objUBOs[i] != VK_NULL_HANDLE)
                vkDestroyBuffer(device, objUBOs[i], nullptr);
            if (objUBOMem[i] != VK_NULL_HANDLE)
                vkFreeMemory(device, objUBOMem[i], nullptr);
        }
        objUBOs.clear(); objUBOMem.clear(); objUBOMapped.clear();
        if (objPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, objPool, nullptr);
            objPool = VK_NULL_HANDLE;
        }
        objDescSets.clear();
    }

    
    void drawModel(VkCommandBuffer cmd, uint32_t imgIdx, mx::MXModel *mdl,
                   const glm::mat4 &v, const glm::mat4 &p, float t,
                   const glm::vec3 &pos, const glm::vec3 &scl,
                   const glm::vec4 &col) {
        glm::mat4 m(1.0f);
        m = glm::translate(m, pos);
        m = glm::scale(m, scl);
        drawModelMat(cmd, imgIdx, mdl, v, p, t, m, col);
    }

    
    void drawModelTextured(VkCommandBuffer cmd, uint32_t imgIdx, mx::MXModel *mdl,
                           const glm::mat4 &v, const glm::mat4 &p, float t,
                           const glm::vec3 &pos, const glm::vec3 &scl,
                           const glm::vec4 &col) {
        if (objDrawIndex >= MAX_DRAWS) return;
        if (tableTexImageView == VK_NULL_HANDLE) {
            
            drawModel(cmd, imgIdx, mdl, v, p, t, pos, scl, col);
            return;
        }
        size_t slot = imgIdx * MAX_DRAWS + objDrawIndex;
        objDrawIndex++;

        glm::mat4 m(1.0f);
        m = glm::translate(m, pos);
        m = glm::scale(m, scl);

        mx::UniformBufferObject ubo{};
        ubo.model  = m;
        ubo.view   = v;
        ubo.proj   = p;
        ubo.params = glm::vec4(t, (float)swapChainExtent.width,
                               (float)swapChainExtent.height, 0.0f);
        ubo.color  = col;
        memcpy(objUBOMapped[slot], &ubo, sizeof(ubo));

        
        VkDescriptorImageInfo imgInfo{};
        imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imgInfo.imageView   = tableTexImageView;
        imgInfo.sampler     = textureSampler;

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = objDescSets[slot];
        write.dstBinding      = 0;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo      = &imgInfo;
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, 0, 1, &objDescSets[slot], 0, nullptr);
        mdl->draw(cmd);
    }

    void drawModelMat(VkCommandBuffer cmd, uint32_t imgIdx, mx::MXModel *mdl,
                      const glm::mat4 &v, const glm::mat4 &p, float t,
                      const glm::mat4 &m, const glm::vec4 &col) {
        if (objDrawIndex >= MAX_DRAWS) return;   

        size_t slot = imgIdx * MAX_DRAWS + objDrawIndex;
        objDrawIndex++;

        mx::UniformBufferObject ubo{};
        ubo.model  = m;
        ubo.view   = v;
        ubo.proj   = p;
        ubo.params = glm::vec4(t, (float)swapChainExtent.width,
                               (float)swapChainExtent.height, 0.0f);
        ubo.color  = col;
        memcpy(objUBOMapped[slot], &ubo, sizeof(ubo));

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, 0, 1, &objDescSets[slot], 0, nullptr);
        mdl->draw(cmd);
    }

    
    void rackBalls() {
        balls[0] = {};
        balls[0].pos    = glm::vec2(-TABLE_HALF_W * 0.5f, 0.0f);
        balls[0].active = true;
        balls[0].number = 0;

        int order[15] = {1,2,3,8,4,5,6,7,9,10,11,12,13,14,15};
        float sx = TABLE_HALF_W * 0.3f;
        float sp = BALL_RADIUS * 2.1f;
        int idx = 0;
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col <= row; col++) {
                if (idx >= 15) break;
                int bn = order[idx];
                balls[bn] = {};
                balls[bn].pos    = glm::vec2(sx + row * sp * 0.866f,
                                             (col - row * 0.5f) * sp);
                balls[bn].active = true;
                balls[bn].number = bn;
                balls[bn].spinAngle = randFloat(0, 2*M_PI);
                idx++;
            }
        }
    }

    void resetGame() {
        rackBalls();
        phase = GamePhase::Aiming;
        cueAngle = 0.0f;
        chargeAmount = 0.0f;
        shotCount = 0;
        lastTime  = SDL_GetTicks() / 1000.0f;
    }

    void handleAiming(float dt) {
        const Uint8 *k = SDL_GetKeyboardState(nullptr);
        if (k[SDL_SCANCODE_LEFT])  cueAngle -= 1.5f * dt;
        if (k[SDL_SCANCODE_RIGHT]) cueAngle += 1.5f * dt;
    }

    void handleCharging(float dt) {
        const Uint8 *k = SDL_GetKeyboardState(nullptr);
        if (k[SDL_SCANCODE_LEFT])  cueAngle -= 1.5f * dt;
        if (k[SDL_SCANCODE_RIGHT]) cueAngle += 1.5f * dt;
        chargeAmount += MAX_POWER * 0.8f * dt;
        if (chargeAmount > MAX_POWER) chargeAmount = MAX_POWER;
    }

    void handlePlacing(float dt) {
        const Uint8 *k = SDL_GetKeyboardState(nullptr);
        float s = 3.0f * dt;
        if (k[SDL_SCANCODE_LEFT])  balls[0].pos.x -= s;
        if (k[SDL_SCANCODE_RIGHT]) balls[0].pos.x += s;
        if (k[SDL_SCANCODE_UP])    balls[0].pos.y -= s;
        if (k[SDL_SCANCODE_DOWN])  balls[0].pos.y += s;
        float m = BALL_RADIUS + 0.1f;
        balls[0].pos.x = glm::clamp(balls[0].pos.x, -TABLE_HALF_W + m, TABLE_HALF_W - m);
        balls[0].pos.y = glm::clamp(balls[0].pos.y, -TABLE_HALF_H + m, TABLE_HALF_H - m);
    }

    
    void updatePhysics(float dt) {
        float maxSpeed = 0.0f;
        for (auto &b : balls) {
            if (b.active && !b.pocketed) {
                float spd = glm::length(b.vel);
                if (spd > maxSpeed) maxSpeed = spd;
            }
        }
        float maxMovePerStep = BALL_RADIUS * 0.75f;
        int steps = std::max(8, (int)std::ceil(maxSpeed * dt * 60.0f / maxMovePerStep));
        float sub = dt / (float)steps;
        float stepFriction = std::pow(FRICTION, 4.0f / (float)steps);
        for (int s = 0; s < steps; s++) {
            for (auto &b : balls) {
                if (!b.active || b.pocketed) continue;
                b.pos += b.vel * sub * 60.0f;
            }
            for (int i = 0; i < NUM_BALLS; i++) {
                if (!balls[i].active || balls[i].pocketed) continue;
                for (int j = i+1; j < NUM_BALLS; j++) {
                    if (!balls[j].active || balls[j].pocketed) continue;
                    resolveBall(balls[i], balls[j]);
                }
            }
            for (auto &b : balls) {
                if (!b.active || b.pocketed) continue;
                resolveWall(b);
            }
            for (auto &b : balls) {
                if (!b.active || b.pocketed) continue;
                checkPocket(b);
            }
            for (auto &b : balls) {
                if (!b.active || b.pocketed) continue;
                b.vel *= stepFriction;
                if (glm::length(b.vel) < MIN_SPEED) b.vel = glm::vec2(0);
            }
        }
    }

    void resolveBall(PoolBall &a, PoolBall &b) {
        glm::vec2 d = b.pos - a.pos;
        float dist = glm::length(d);
        float minD = BALL_RADIUS * 2.0f;
        if (dist < minD && dist > 0.0001f) {
            glm::vec2 n = d / dist;
            float ov = minD - dist;
            a.pos -= n * (ov * 0.5f);
            b.pos += n * (ov * 0.5f);
            float rv = glm::dot(a.vel - b.vel, n);
            if (rv > 0) { a.vel -= n * rv; b.vel += n * rv; }
        }
    }

    void resolveWall(PoolBall &b) {
        float l = -TABLE_HALF_W + BALL_RADIUS, r = TABLE_HALF_W - BALL_RADIUS;
        float t = -TABLE_HALF_H + BALL_RADIUS, bt = TABLE_HALF_H - BALL_RADIUS;
        if (b.pos.x < l) { b.pos.x = l; b.vel.x = -b.vel.x * 0.8f; }
        if (b.pos.x > r) { b.pos.x = r; b.vel.x = -b.vel.x * 0.8f; }
        if (b.pos.y < t) { b.pos.y = t; b.vel.y = -b.vel.y * 0.8f; }
        if (b.pos.y > bt){ b.pos.y = bt; b.vel.y = -b.vel.y * 0.8f; }
    }

    void checkPocket(PoolBall &b) {
        for (int i = 0; i < 6; i++) {
            if (glm::length(b.pos - POCKETS[i]) < POCKET_R) {
                b.pocketed = true;
                b.vel = glm::vec2(0);
                return;
            }
        }
    }

    bool anyBallMoving() const {
        for (auto &b : balls)
            if (b.active && !b.pocketed && b.isMoving()) return true;
        return false;
    }
    bool allObjectBallsPocketed() const {
        for (int i = 1; i < NUM_BALLS; i++)
            if (balls[i].active && !balls[i].pocketed) return false;
        return true;
    }
    void checkGameOver() {
        if (allObjectBallsPocketed()) phase = GamePhase::GameOver;
    }
};


int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
        PoolWindow window(args.path, args.width, args.height, args.fullscreen);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
