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
#include <fstream>
#include <sstream>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
static constexpr float TABLE_W = 12.0f;
static constexpr float TABLE_H = 6.0f;
static constexpr float TABLE_HALF_W = TABLE_W / 2.0f;
static constexpr float TABLE_HALF_H = TABLE_H / 2.0f;
static constexpr float BUMPER_W = 0.3f;
static constexpr float POCKET_R = 0.25f;
static constexpr float BALL_RADIUS = 0.18f;
static constexpr float CUE_LENGTH = 2.5f;
static constexpr int NUM_BALLS = 16;
static constexpr float FRICTION = 0.9988f;
static constexpr float MIN_SPEED = 0.001f;
static constexpr float MAX_POWER = 1.0f;
static constexpr int MAX_DRAWS = 48;
static constexpr float PKT_INS = 0.25f;
static const glm::vec2 POCKETS[6] = {
    { -TABLE_HALF_W + PKT_INS, TABLE_HALF_H - PKT_INS },
    { 0.0f, TABLE_HALF_H - 0.15f },
    { TABLE_HALF_W - PKT_INS, TABLE_HALF_H - PKT_INS },
    { -TABLE_HALF_W + PKT_INS, -TABLE_HALF_H + PKT_INS },
    { 0.0f, -TABLE_HALF_H + 0.15f },
    { TABLE_HALF_W - PKT_INS, -TABLE_HALF_H + PKT_INS },
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
enum class GameScreen { Intro, Scores, Game, Start };
static float randFloat(float mn, float mx) {
    static std::random_device rd;
    static std::default_random_engine eng(rd());
    std::uniform_real_distribution<float> d(mn, mx);
    return d(eng);
}
struct PoolBall {
    glm::vec2 pos{0.0f};
    glm::vec2 vel{0.0f};
    bool active = true;
    bool pocketed = false;
    int number = 0;
    float spinAngle = 0.0f;
    bool isMoving() const { return glm::length(vel) > MIN_SPEED; }
};
enum class GamePhase { Aiming, Charging, Rolling, Placing, GameOver };
static constexpr float SINK_DURATION = 0.45f;
struct SinkAnim {
    glm::vec2 pocketPos;
    glm::vec3 color;
    float spinAngle;
    float timer = 0.0f;
};
struct Score {
    std::string name;
    int shots; 
    Score() : name{}, shots{0} {}
    Score(const std::string &n, int s) : name(n), shots(s) {}
};
class HighScores {
public:
    HighScores() { read(); }
    ~HighScores() { write(); }
    void addScore(const std::string &name, int shots) {
        entries.push_back({name, shots});
        sort();
        if (entries.size() > 10) entries.resize(10);
    }
    void sort() {
        std::sort(entries.begin(), entries.end(),
            [](const Score &a, const Score &b){ return a.shots < b.shots; });
    }
    bool qualifies(int shots) const {
        if (entries.size() < 10) return true;
        return shots < entries.back().shots;
    }
    const std::vector<Score>& list() const { return entries; }
    void read() {
        std::fstream f("./pool_scores.dat", std::ios::in);
        if (!f.is_open()) { initDefaults(); return; }
        std::string line;
        int count = 0;
        while (std::getline(f, line) && count < 10) {
            auto p = line.find(':');
            if (p != std::string::npos) {
                entries.push_back({line.substr(0, p),
                    (int)std::strtol(line.substr(p+1).c_str(), nullptr, 10)});
                count++;
            }
        }
        sort();
    }
    void write() const {
        std::fstream f("./pool_scores.dat", std::ios::out);
        if (!f.is_open()) return;
        for (auto &e : entries) f << e.name << ":" << e.shots << "\n";
    }
private:
    void initDefaults() {
        for (int i = 1; i <= 10; i++)
            entries.push_back({"Anonymous", i * 20});
    }
    std::vector<Score> entries;
};
class PoolWindow : public mx::VKWindow {
    GameScreen screen = GameScreen::Intro;
public:
    PoolWindow(const std::string &path, int wx, int wy, bool full)
        : mx::VKWindow("-[ 3D Pool / Vulkan ]-", wx, wy, full) {
        setPath(path);
    }
    ~PoolWindow() override = default;
    void initVulkan() override {
        mx::VKWindow::initVulkan();
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
        for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (SDL_IsGameController(i)) {
                gameController = SDL_GameControllerOpen(i);
                if (gameController) {
                    SDL_Log("Controller opened: %s", SDL_GameControllerName(gameController));
                    break;
                }
            }
        }
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
        tableModel.reset(new mx::MXModel());
        tableModel->load(util.path + "/data/pooltable_felt.mxmod.z", 1.0f);
        tableModel->upload(device, physicalDevice, commandPool, graphicsQueue);
        tableWoodModel.reset(new mx::MXModel());
        tableWoodModel->load(util.path + "/data/pooltable_wood.mxmod.z", 1.0f);
        tableWoodModel->upload(device, physicalDevice, commandPool, graphicsQueue);
        tablePocketModel.reset(new mx::MXModel());
        tablePocketModel->load(util.path + "/data/pooltable_pocket.mxmod.z", 1.0f);
        tablePocketModel->upload(device, physicalDevice, commandPool, graphicsQueue);
        backgroundSprite = createSprite(util.path + "/data/background.png", "", "");
        startscreenSprite = createSprite(util.path + "/data/start.png", "", "");
        scoresBackground = createSprite(util.path + "/data/scores.png", "", "");
        introSprite = createSprite(util.path + "/data/logo.png", util.path + "/data/sprite_vert.spv", util.path + "/data/bend_dir.spv");
    }
    void procIntro() {
        uint32_t cur_time = SDL_GetTicks();
        static uint32_t start_t = 0;
        if(start_t == 0)
            start_t = SDL_GetTicks();
        uint32_t elapsed = cur_time - start_t;
        constexpr uint32_t TOTAL = 5000;
        constexpr uint32_t FADE_DUR = 1500;
        if(elapsed >= TOTAL) {
            start_t = 0;
            setScreen(GameScreen::Start);
            return;
        }
        float fadeOut = 1.0f;
        if(elapsed > TOTAL - FADE_DUR) {
            fadeOut = 1.0f - static_cast<float>(elapsed - (TOTAL - FADE_DUR)) / static_cast<float>(FADE_DUR);
        }
        if(fadeOut < 1.0f && startscreenSprite != nullptr) {
            startscreenSprite->setShaderParams(1.0f, 1.0f, 1.0f, 1.0f);
            startscreenSprite->drawSpriteRect(0, 0, getWidth(), getHeight());
        }
        if(introSprite != nullptr) {
            float time = static_cast<float>(SDL_GetTicks() / 1000.0f);
            introSprite->setShaderParams(fadeOut, 1.0f, 1.0f, time);
            introSprite->drawSpriteRect(0, 0, getWidth(), getHeight());
        }
    }
    void procStart() {
        if(startscreenSprite != nullptr) {
            startscreenSprite->setShaderParams(1.0f, 1.0f, 1.0f, 1.0f);
            startscreenSprite->drawSpriteRect(0, 0, getWidth(), getHeight());
        }
        const char *hint = "ENTER / A  -  Play      SPACE /Y  -  Scores      ESC / Back  -  Quit";
        int tw, th;
        getTextDimensions(hint, tw, th);
        printText(hint, w / 2 - tw / 2, (h - th * 3)+20, {220, 220, 100, 255});
        const Uint8 *keys = SDL_GetKeyboardState(nullptr);
        if(keys[SDL_SCANCODE_RETURN]) {
            resetGame();
            setScreen(GameScreen::Game);
        } else if(keys[SDL_SCANCODE_SPACE]) {
            setScreen(GameScreen::Scores);
        }
    }
    void procGame() {
        float now = SDL_GetTicks() / 1000.0f;
        float dt = now - lastTime;
        if (dt > 0.05f) dt = 0.05f;
        lastTime = now;
        switch (phase) {
        case GamePhase::Aiming: handleAiming(dt); break;
        case GamePhase::Charging: handleCharging(dt); break;
        case GamePhase::Rolling:
            updatePhysics(dt);
            if (!anyBallMoving()) {
                if (balls[0].pocketed) {
                    phase = GamePhase::Placing;
                    balls[0].active = true;
                    balls[0].pocketed = false;
                    balls[0].pos = glm::vec2(-TABLE_HALF_W * 0.5f, 0.0f);
                    balls[0].vel = glm::vec2(0.0f);
                } else {
                    phase = GamePhase::Aiming;
                }
                checkGameOver();
            }
            break;
        case GamePhase::Placing: handlePlacing(dt); break;
        case GamePhase::GameOver: break;
        }
        for (auto &b : balls) {
            if (b.active && b.isMoving()) {
                b.spinAngle += glm::length(b.vel) * 5.0f * dt;
            }
        }
        for (auto &a : sinkAnims) a.timer += dt;
        sinkAnims.erase(
            std::remove_if(sinkAnims.begin(), sinkAnims.end(),
                           [](const SinkAnim &a){ return a.timer >= SINK_DURATION; }),
            sinkAnims.end());
        {
            const Uint8 *k = SDL_GetKeyboardState(nullptr);
            if (k[SDL_SCANCODE_A]) camAngle -= 1.2f * dt;
            if (k[SDL_SCANCODE_S]) camAngle += 1.2f * dt;
            if (k[SDL_SCANCODE_W]) camZoom -= 5.0f * dt;
            if (k[SDL_SCANCODE_E]) camZoom += 5.0f * dt;
            camZoom = glm::clamp(camZoom, 5.0f, 25.0f);
        }
        if (gameController) {
            constexpr float DEADZONE = 0.15f;
            auto readAxis = [&](SDL_GameControllerAxis a) -> float {
                return SDL_GameControllerGetAxis(gameController, a) / 32767.0f;
            };
            float rx = readAxis(SDL_CONTROLLER_AXIS_RIGHTX);
            float ry = readAxis(SDL_CONTROLLER_AXIS_RIGHTY);
            if (fabsf(rx) > DEADZONE) camAngle += rx * 1.5f * dt;
            if (fabsf(ry) > DEADZONE) camZoom += ry * 5.0f * dt;
            camZoom = glm::clamp(camZoom, 5.0f, 25.0f);
            float lx = readAxis(SDL_CONTROLLER_AXIS_LEFTX);
            float ly = readAxis(SDL_CONTROLLER_AXIS_LEFTY);
            if (phase == GamePhase::Aiming || phase == GamePhase::Charging) {
                if (fabsf(lx) > DEADZONE) cueAngle += lx * 1.5f * dt;
            } else if (phase == GamePhase::Placing) {
                float s = 3.0f * dt;
                glm::vec2 camRight( cosf(camAngle), -sinf(camAngle));
                glm::vec2 camFwd (-sinf(camAngle), -cosf(camAngle));
                if (fabsf(lx) > DEADZONE) balls[0].pos += camRight * (lx * s);
                if (fabsf(ly) > DEADZONE) balls[0].pos -= camFwd * (ly * s);
                float m = BALL_RADIUS + 0.1f;
                balls[0].pos.x = glm::clamp(balls[0].pos.x, -TABLE_HALF_W + m, TABLE_HALF_W - m);
                balls[0].pos.y = glm::clamp(balls[0].pos.y, -TABLE_HALF_H + m, TABLE_HALF_H - m);
            }
        }
        printText("Shots: " + std::to_string(shotCount), 15, 45, {255, 255, 0, 255});
        int rem = 0;
        for (int i = 1; i < NUM_BALLS; i++)
            if (balls[i].active && !balls[i].pocketed) rem++;
        printText("Balls: " + std::to_string(rem), 15, 75, {200, 200, 200, 255});
        if (phase == GamePhase::Aiming)
            printText("Arrows/L-Stick: Aim | Space/A/B: Charge & Shoot | R-Stick: Cam", 15, h - 40, {180, 180, 180, 255});
        else if (phase == GamePhase::Charging) {
            int pct = static_cast<int>(chargeAmount / MAX_POWER * 100.0f);
            printText("Power: " + std::to_string(pct) + "%", 15, 105,
                      {255, static_cast<Uint8>(255 - pct * 2), 0, 255});
        } else if (phase == GamePhase::Placing)
            printText("Arrows/L-Stick: move cue ball | Enter/A/B: place", 15, h - 40, {255, 100, 100, 255});
        else if (phase == GamePhase::GameOver) {
        }
    }
    void setScreen(GameScreen scr) {
        if (screen == GameScreen::Scores && scr != GameScreen::Scores) {
            setFont("font.ttf", 24);
            lastScoreFontSize = 0;
        }
        screen = scr;
    }
    void goToScoresScreen() {
        finalScore = shotCount;
        playerName = "";
        enteringName = highScores.qualifies(finalScore);
        setScreen(GameScreen::Scores);
    }
    void procScores() {
        if (scoresBackground != nullptr) {
            scoresBackground->setShaderParams(1.0f, 1.0f, 1.0f, 1.0f);
            scoresBackground->drawSpriteRect(0, 0, getWidth(), getHeight());
        }

        // Felt area as fractions of the window, matched to the start.png image layout.
        // Adjust these if the image layout differs.
        int feltL  = (int)(w * 0.125f);   // left rail edge
        int feltT  = (int)(h * 0.19f);    // below the decorative HIGH SCORES banner
        int feltB  = (int)(h * 0.87f);    // above the bottom rail
        int feltH  = feltB - feltT;
        int feltCX = (int)(w * 0.48f);    // horizontal centre of felt

        // Scale font so 10 entries + prompt fit comfortably inside the felt
        int fs = std::max(10, feltH / 15);
        if (fs != lastScoreFontSize) {
            setFont("font.ttf", fs);
            lastScoreFontSize = fs;
        }
        int lineH = fs + fs / 3;  // line height with inter-line spacing

        const auto &scoreList = highScores.list();
        for (size_t i = 0; i < scoreList.size() && i < 10; i++) {
            std::ostringstream ss;
            ss << (i+1) << ". " << scoreList[i].name << "  " << scoreList[i].shots << " shots";
            std::string row = ss.str();
            SDL_Color col = {255, 255, 255, 255};
            if (enteringName && finalScore > 0) {
                int rank = (int)scoreList.size();
                for (int j = 0; j < (int)scoreList.size(); j++) {
                    if (finalScore < scoreList[j].shots) { rank = j; break; }
                }
                if ((int)i == rank) col = {0, 255, 0, 255};
            }
            printText(row, feltL + fs / 2, feltT + (int)i * lineH, col);
        }

        if (enteringName) {
            int entryY = feltT + 10 * lineH + lineH / 2;
            std::ostringstream ss;
            ss << "Your score: " << finalScore << " shots";
            std::string ys = ss.str();
            int tw, th;
            getTextDimensions(ys, tw, th);
            printText(ys, feltCX - tw / 2, entryY, {255, 255, 0, 255});
            std::string dn = playerName + "_";
            getTextDimensions(dn, tw, th);
            printText(dn, feltCX - tw / 2, entryY + lineH, {0, 255, 255, 255});
            const char *hint = "ENTER to confirm  |  BACKSPACE to delete";
            getTextDimensions(hint, tw, th);
            printText(hint, feltCX - tw / 2, entryY + lineH * 2, {200, 200, 200, 255});
        } else {
            const char *ret = "Press ENTER to return to intro";
            int tw, th;
            getTextDimensions(ret, tw, th);
            printText(ret, feltCX - tw / 2, feltB - lineH, {255, 255, 0, 255});
        }
    }
    void proc() override {
        switch(screen) {
            case GameScreen::Intro:
                procIntro();
                break;
            case GameScreen::Start:
                procStart();
                break;
            case GameScreen::Game:
                procGame();
                break;
            case GameScreen::Scores:
                procScores();
                break;
        }
    }
    void drawIntro() {
        VKWindow::draw();
    }
    void drawStart() {
        VKWindow::draw();
    }
    void drawScores() {
        VKWindow::draw();
    }
    void drawGame() {
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
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass = renderPass;
        rpInfo.framebuffer = swapChainFramebuffers[imageIndex];
        rpInfo.renderArea.offset = {0, 0};
        rpInfo.renderArea.extent = swapChainExtent;
        std::array<VkClearValue, 2> cv{};
        cv[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        cv[1].depthStencil = {1.0f, 0};
        rpInfo.clearValueCount = static_cast<uint32_t>(cv.size());
        rpInfo.pClearValues = cv.data();
        VkCommandBuffer cmd = commandBuffers[imageIndex];
        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport vp{}; vp.width = (float)swapChainExtent.width;
        vp.height = (float)swapChainExtent.height; vp.maxDepth = 1.0f;
        VkRect2D sc{}; sc.extent = swapChainExtent;
        vkCmdSetViewport(cmd, 0, 1, &vp);
        vkCmdSetScissor(cmd, 0, 1, &sc);
        if (spritePipeline != VK_NULL_HANDLE && backgroundSprite != nullptr) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline);
            backgroundSprite->setShaderParams(1.0f, 1.0f, 1.0f, 1.0f);
            backgroundSprite->drawSpriteRect(0, 0,
                static_cast<int>(swapChainExtent.width),
                static_cast<int>(swapChainExtent.height));
            backgroundSprite->renderSprites(cmd, spritePipelineLayout,
                swapChainExtent.width, swapChainExtent.height);
            backgroundSprite->clearQueue();
        }
        float aspect = vp.width / vp.height;
        float time = SDL_GetTicks() / 1000.0f;
        float camDist = camZoom;
        float camHeight = camZoom * 0.92f;
        glm::vec3 camPos = glm::vec3(sinf(camAngle) * camDist, camHeight, cosf(camAngle) * camDist);
        glm::vec3 camTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::mat4 viewMat = glm::lookAt(camPos, camTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projMat = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        projMat[1][1] *= -1;
        VkPipeline pipe = useWireFrame ? graphicsPipelineMatrix : graphicsPipeline;
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
        objDrawIndex = 0;
        drawModelTextured(cmd, imageIndex, tableModel.get(), viewMat, projMat, time,
                  glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(1.0f, 1.0f, 1.0f),
                  glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        drawModel(cmd, imageIndex, tableWoodModel.get(), viewMat, projMat, time,
                  glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(1.0f, 1.0f, 1.0f),
                  glm::vec4(0.4f, 0.22f, 0.05f, 1.0f));
        drawModel(cmd, imageIndex, tablePocketModel.get(), viewMat, projMat, time,
                  glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(1.0f, 1.0f, 1.0f),
                  glm::vec4(0.08f, 0.08f, 0.08f, 1.0f));
        for (int i = 0; i < NUM_BALLS; i++) {
            if (!balls[i].active || balls[i].pocketed) continue;
            glm::mat4 m(1.0f);
            m = glm::translate(m, glm::vec3(balls[i].pos.x, BALL_RADIUS, balls[i].pos.y));
            m = glm::rotate(m, balls[i].spinAngle, glm::vec3(0.0f, 1.0f, 0.0f));
            m = glm::scale(m, glm::vec3(BALL_RADIUS));
            drawModelMat(cmd, imageIndex, sphereModel.get(), viewMat, projMat, time,
                         m, glm::vec4(BALL_COLORS[i], 1.0f));
        }
        for (auto &anim : sinkAnims) {
            float t = anim.timer / SINK_DURATION;
            float y = glm::mix(BALL_RADIUS, -BALL_RADIUS * 3.5f, t);
            float sc = BALL_RADIUS * (1.0f - t * 0.75f);
            glm::mat4 m(1.0f);
            m = glm::translate(m, glm::vec3(anim.pocketPos.x, y, anim.pocketPos.y));
            m = glm::rotate(m, anim.spinAngle + t * 6.0f, glm::vec3(0.0f, 1.0f, 0.0f));
            m = glm::scale(m, glm::vec3(sc));
            drawModelMat(cmd, imageIndex, sphereModel.get(), viewMat, projMat, time,
                         m, glm::vec4(anim.color, 1.0f));
        }
        if ((phase == GamePhase::Aiming || phase == GamePhase::Charging) && balls[0].active) {
            float offset = (phase == GamePhase::Charging)
                             ? (chargeAmount / MAX_POWER * 1.0f) : 0.0f;
            float stickDist = BALL_RADIUS + 0.3f + offset;
            float worldCueAngle = cueAngle + camAngle;
            glm::vec2 dir(cosf(worldCueAngle), sinf(worldCueAngle));
            glm::vec2 sc2 = balls[0].pos - dir * (stickDist + CUE_LENGTH * 0.5f);
            glm::mat4 m(1.0f);
            m = glm::translate(m, glm::vec3(sc2.x, BALL_RADIUS + 0.05f, sc2.y));
            m = glm::rotate(m, -worldCueAngle, glm::vec3(0.0f, 1.0f, 0.0f));
            m = glm::scale(m, glm::vec3(CUE_LENGTH * 0.5f, 0.03f, 0.03f));
            float pct = chargeAmount / MAX_POWER;
            glm::vec4 cueCol = (phase == GamePhase::Charging)
                ? glm::vec4(0.55f + pct*0.45f, 0.27f*(1-pct), 0.07f*(1-pct), 1)
                : glm::vec4(0.55f, 0.27f, 0.07f, 1.0f);
            drawModelMat(cmd, imageIndex, cubeModel.get(), viewMat, projMat, time, m, cueCol);
            float aimLen = 2.0f;
            glm::vec2 ac = balls[0].pos + dir * (BALL_RADIUS + aimLen * 0.5f);
            glm::mat4 am(1.0f);
            am = glm::translate(am, glm::vec3(ac.x, BALL_RADIUS + 0.06f, ac.y));
            am = glm::rotate(am, -worldCueAngle, glm::vec3(0.0f, 1.0f, 0.0f));
            am = glm::scale(am, glm::vec3(aimLen * 0.5f, 0.008f, 0.015f));
            drawModelMat(cmd, imageIndex, cubeModel.get(), viewMat, projMat, time,
                         am, glm::vec4(1, 1, 0, 1.0f));
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
    void draw() override {
        switch(screen) {
            case GameScreen::Intro:
                drawIntro();
                break;
            case GameScreen::Game:
                drawGame();
                break;
            case GameScreen::Scores:
                drawScores();
                break;
            case GameScreen::Start:
                drawStart();
                break;
        }
    }
    void event(SDL_Event &e) override {
        if (e.type == SDL_QUIT) { quit(); return; }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
            if (screen == GameScreen::Scores) {
                setScreen(GameScreen::Intro);
            } else {
                quit();
            }
            return;
        }
        if (e.type == SDL_TEXTINPUT && screen == GameScreen::Scores && enteringName) {
            if (playerName.length() < 15) playerName += e.text.text;
            return;
        }
        if (e.type == SDL_KEYDOWN && screen == GameScreen::Scores) {
            if (enteringName) {
                if (e.key.keysym.sym == SDLK_RETURN && !playerName.empty()) {
                    highScores.addScore(playerName, finalScore);
                    highScores.write();
                    enteringName = false;
                } else if (e.key.keysym.sym == SDLK_BACKSPACE && !playerName.empty()) {
                    playerName.pop_back();
                }
            } else {
                if (e.key.keysym.sym == SDLK_RETURN)
                    setScreen(GameScreen::Intro);
            }
            return;
        }
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
            case SDLK_r: if (screen == GameScreen::Game) resetGame(); break;
            case SDLK_p: setWireFrame(!getWireFrame()); break;
            case SDLK_SPACE:
                if (screen == GameScreen::Game && phase == GamePhase::Aiming) {
                    phase = GamePhase::Charging;
                    chargeAmount = 0.0f;
                }
                break;
            case SDLK_RETURN:
                if (screen == GameScreen::Game && phase == GamePhase::Placing) {
                    bool ok = true;
                    for (int i = 1; i < NUM_BALLS; i++) {
                        if (!balls[i].active || balls[i].pocketed) continue;
                        if (glm::length(balls[0].pos - balls[i].pos) < BALL_RADIUS * 2.5f)
                        { ok = false; break; }
                    }
                    if (ok) phase = GamePhase::Aiming;
                }
                break;
            default: break;
            }
        }
        if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_SPACE
            && screen == GameScreen::Game && phase == GamePhase::Charging) {
            float worldCueAngle = cueAngle + camAngle;
            glm::vec2 dir(cosf(worldCueAngle), sinf(worldCueAngle));
            balls[0].vel = dir * chargeAmount;
            phase = GamePhase::Rolling;
            shotCount++;
            chargeAmount = 0.0f;
        }
        if (e.type == SDL_CONTROLLERDEVICEADDED && !gameController) {
            gameController = SDL_GameControllerOpen(e.cdevice.which);
            if (gameController)
                SDL_Log("Controller connected: %s", SDL_GameControllerName(gameController));
        }
        if (e.type == SDL_CONTROLLERDEVICEREMOVED && gameController) {
            SDL_Joystick *j = SDL_GameControllerGetJoystick(gameController);
            if (j && SDL_JoystickInstanceID(j) == e.cdevice.which) {
                SDL_GameControllerClose(gameController);
                gameController = nullptr;
                SDL_Log("Controller disconnected");
            }
        }
        if (e.type == SDL_CONTROLLERBUTTONDOWN) {
            if (screen == GameScreen::Intro) {
                if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A ||
                    e.cbutton.button == SDL_CONTROLLER_BUTTON_START)
                    setScreen(GameScreen::Start);
                return;
            }
            if (screen == GameScreen::Scores) {
                if (!enteringName) {
                    if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A ||
                        e.cbutton.button == SDL_CONTROLLER_BUTTON_B ||
                        e.cbutton.button == SDL_CONTROLLER_BUTTON_START)
                        setScreen(GameScreen::Intro);
                }
                return;
            }
            if (screen == GameScreen::Start) {
                if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A ||
                    e.cbutton.button == SDL_CONTROLLER_BUTTON_START) {
                    resetGame();
                    setScreen(GameScreen::Game);
                } else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_Y) {
                    setScreen(GameScreen::Scores);
                } else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) {
                    quit();
                }
                return;
            }
            switch (e.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_BACK:
                quit();
                return;
            case SDL_CONTROLLER_BUTTON_A:
            case SDL_CONTROLLER_BUTTON_B:
                if (screen == GameScreen::Game) {
                    if (phase == GamePhase::Aiming) {
                        phase = GamePhase::Charging;
                        chargeAmount = 0.0f;
                        ctrlChargeButton = e.cbutton.button;
                    } else if (phase == GamePhase::Placing) {
                        bool ok = true;
                        for (int i = 1; i < NUM_BALLS; i++) {
                            if (!balls[i].active || balls[i].pocketed) continue;
                            if (glm::length(balls[0].pos - balls[i].pos) < BALL_RADIUS * 2.5f)
                            { ok = false; break; }
                        }
                        if (ok) phase = GamePhase::Aiming;
                    }
                }
                break;
            default: break;
            }
        }
        if (e.type == SDL_CONTROLLERBUTTONUP) {
            if ((e.cbutton.button == SDL_CONTROLLER_BUTTON_A ||
                 e.cbutton.button == SDL_CONTROLLER_BUTTON_B) &&
                 e.cbutton.button == ctrlChargeButton &&
                 screen == GameScreen::Game &&
                 phase == GamePhase::Charging) {
                float worldCueAngle = cueAngle + camAngle;
                glm::vec2 dir(cosf(worldCueAngle), sinf(worldCueAngle));
                balls[0].vel = dir * chargeAmount;
                phase = GamePhase::Rolling;
                shotCount++;
                chargeAmount = 0.0f;
                ctrlChargeButton = -1;
            }
        }
    }
    void cleanup() override {
        if (gameController) {
            SDL_GameControllerClose(gameController);
            gameController = nullptr;
        }
        if (sphereModel) sphereModel->cleanup(device);
        if (cylinderModel) cylinderModel->cleanup(device);
        if (cubeModel) cubeModel->cleanup(device);
        if (tableModel) tableModel->cleanup(device);
        if (tableWoodModel) tableWoodModel->cleanup(device);
        if (tablePocketModel) tablePocketModel->cleanup(device);
        cleanupTableTexture();
        cleanupObjectPool();
        mx::VKWindow::cleanup();
    }
private:
    std::unique_ptr<mx::MXModel> sphereModel;
    std::unique_ptr<mx::MXModel> cylinderModel;
    std::unique_ptr<mx::MXModel> cubeModel;
    std::unique_ptr<mx::MXModel> tableModel;
    std::unique_ptr<mx::MXModel> tableWoodModel;
    std::unique_ptr<mx::MXModel> tablePocketModel;
    PoolBall balls[NUM_BALLS];
    std::vector<SinkAnim> sinkAnims;
    GamePhase phase = GamePhase::Aiming;
    float cueAngle = 0.0f;
    float chargeAmount = 0.0f;
    int shotCount = 0;
    float lastTime = 0.0f;
    float camAngle = 0.4f;
    float camZoom = 13.0f;
    SDL_GameController *gameController = nullptr;
    int ctrlChargeButton = -1;
    VkImage tableTexImage = VK_NULL_HANDLE;
    VkDeviceMemory tableTexMemory = VK_NULL_HANDLE;
    VkImageView tableTexImageView = VK_NULL_HANDLE;
    mx::VKSprite *backgroundSprite = nullptr;
    mx::VKSprite *startscreenSprite = nullptr;
    mx::VKSprite *introSprite = nullptr;
    mx::VKSprite *scoresBackground = nullptr;
    HighScores highScores;
    bool enteringName = false;
    std::string playerName;
    int finalScore = 0;
    int lastScoreFontSize = 0;
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
    size_t swapCount = 0;
    VkDescriptorPool objPool = VK_NULL_HANDLE;
    std::vector<VkBuffer> objUBOs;
    std::vector<VkDeviceMemory> objUBOMem;
    std::vector<void*> objUBOMapped;
    std::vector<VkDescriptorSet> objDescSets;
    int objDrawIndex = 0;
    void createObjectPool() {
        swapCount = swapChainImages.size();
        size_t total = MAX_DRAWS * swapCount;
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(total);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(total);
        VkDescriptorPoolCreateInfo pi{};
        pi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pi.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        pi.pPoolSizes = poolSizes.data();
        pi.maxSets = static_cast<uint32_t>(total);
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
        ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool = objPool;
        ai.descriptorSetCount = static_cast<uint32_t>(total);
        ai.pSetLayouts = layouts.data();
        objDescSets.resize(total);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &ai, objDescSets.data()));
        for (size_t i = 0; i < total; i++) {
            VkDescriptorImageInfo imgInfo{};
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imgInfo.imageView = textureImageView;
            imgInfo.sampler = textureSampler;
            VkDescriptorBufferInfo bufInfo{};
            bufInfo.buffer = objUBOs[i];
            bufInfo.offset = 0;
            bufInfo.range = uboSize;
            std::array<VkWriteDescriptorSet, 2> writes{};
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = objDescSets[i];
            writes[0].dstBinding = 0;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[0].descriptorCount = 1;
            writes[0].pImageInfo = &imgInfo;
            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = objDescSets[i];
            writes[1].dstBinding = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[1].descriptorCount = 1;
            writes[1].pBufferInfo = &bufInfo;
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
        ubo.model = m;
        ubo.view = v;
        ubo.proj = p;
        ubo.params = glm::vec4(t, (float)swapChainExtent.width,
                               (float)swapChainExtent.height, 0.0f);
        ubo.color = col;
        memcpy(objUBOMapped[slot], &ubo, sizeof(ubo));
        VkDescriptorImageInfo imgInfo{};
        imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imgInfo.imageView = tableTexImageView;
        imgInfo.sampler = textureSampler;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = objDescSets[slot];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imgInfo;
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
        ubo.model = m;
        ubo.view = v;
        ubo.proj = p;
        ubo.params = glm::vec4(t, (float)swapChainExtent.width,
                               (float)swapChainExtent.height, 0.0f);
        ubo.color = col;
        memcpy(objUBOMapped[slot], &ubo, sizeof(ubo));
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, 0, 1, &objDescSets[slot], 0, nullptr);
        mdl->draw(cmd);
    }
    void rackBalls() {
        balls[0] = {};
        balls[0].pos = glm::vec2(-TABLE_HALF_W * 0.5f, 0.0f);
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
                balls[bn].pos = glm::vec2(sx + row * sp * 0.866f,
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
        sinkAnims.clear();
        phase = GamePhase::Aiming;
        cueAngle = 0.0f;
        chargeAmount = 0.0f;
        shotCount = 0;
        lastTime = SDL_GetTicks() / 1000.0f;
    }
    void handleAiming(float dt) {
        const Uint8 *k = SDL_GetKeyboardState(nullptr);
        if (k[SDL_SCANCODE_LEFT]) cueAngle -= 1.5f * dt;
        if (k[SDL_SCANCODE_RIGHT]) cueAngle += 1.5f * dt;
    }
    void handleCharging(float dt) {
        const Uint8 *k = SDL_GetKeyboardState(nullptr);
        if (k[SDL_SCANCODE_LEFT]) cueAngle -= 1.5f * dt;
        if (k[SDL_SCANCODE_RIGHT]) cueAngle += 1.5f * dt;
        chargeAmount += MAX_POWER * 0.8f * dt;
        if (chargeAmount > MAX_POWER) chargeAmount = MAX_POWER;
    }
    void handlePlacing(float dt) {
        const Uint8 *k = SDL_GetKeyboardState(nullptr);
        float s = 3.0f * dt;
        glm::vec2 camRight( cosf(camAngle), -sinf(camAngle));
        glm::vec2 camFwd (-sinf(camAngle), -cosf(camAngle));
        if (k[SDL_SCANCODE_LEFT]) balls[0].pos -= camRight * s;
        if (k[SDL_SCANCODE_RIGHT]) balls[0].pos += camRight * s;
        if (k[SDL_SCANCODE_UP]) balls[0].pos += camFwd * s;
        if (k[SDL_SCANCODE_DOWN]) balls[0].pos -= camFwd * s;
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
                int idx = static_cast<int>(&b - &balls[0]);
                SinkAnim anim;
                anim.pocketPos = POCKETS[i];
                anim.color = BALL_COLORS[idx];
                anim.spinAngle = b.spinAngle;
                anim.timer = 0.0f;
                sinkAnims.push_back(anim);
                b.pocketed = true;
                b.vel = glm::vec2(0);
                return;
            }
        }
    }
    bool anyBallMoving() const {
        for (auto &b : balls)
            if (b.active && !b.pocketed && b.isMoving()) return true;
        if (!sinkAnims.empty()) return true;
        return false;
    }
    bool allObjectBallsPocketed() const {
        for (int i = 1; i < NUM_BALLS; i++)
            if (balls[i].active && !balls[i].pocketed) return false;
        return true;
    }
    void checkGameOver() {
        if (allObjectBallsPocketed()) goToScoresScreen();
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
