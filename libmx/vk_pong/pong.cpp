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


class Paddle {
public:
    glm::vec3 position;
    glm::vec3 size;
    float rotationAngle;
    float rotationSpeed;
    bool isRotating;
    
    Paddle(glm::vec3 pos, glm::vec3 sz)
        : position(pos), size(sz), rotationAngle(0.0f), rotationSpeed(0.0f), isRotating(false) {
    }
    
    void update(float deltaTime) {
        if (isRotating) {
            rotationAngle += rotationSpeed * deltaTime;
            if (rotationAngle >= 360.0f) {
                rotationAngle = 0.0f;
                isRotating = false;
            }
        }
    }
    
    void startRotation(float speed) {
        if (!isRotating) {
            rotationSpeed = speed;
            isRotating = true;
        }
    }
    
    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, size);
        return model;
    }
};


class Ball {
public:
    glm::vec3 position;
    glm::vec3 velocity;
    float radius;
    float speed;
    bool hitPaddle1 = false;
    bool hitPaddle2 = false;
    bool hitWall = false;
    glm::vec3 lastImpactPos;
    
    Ball(glm::vec3 pos, glm::vec3 vel, float r)
        : position(pos), velocity(vel), radius(r), speed(glm::length(vel)) {
    }
    
    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, glm::vec3(radius));
        return model;
    }
    
    void resetBall() {
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        float angle = glm::radians(static_cast<float>(rand() % 120 - 60));
        speed = 1.0f;
        
        float vx = cos(angle);
        float vy = sin(angle);
        
        if (std::abs(vx) < 0.5f) {
            vx = (vx < 0) ? -0.5f : 0.5f;
        }
        
        vx *= (rand() % 2 == 0) ? 1.0f : -1.0f;
        
        velocity = glm::normalize(glm::vec3(vx, vy, 0.0f)) * speed;
    }
    
    void update(float deltaTime, Paddle &paddle1, Paddle &paddle2, int &score1, int &score2) {
        
        hitPaddle1 = false;
        hitPaddle2 = false;
        hitWall = false;
        
        position += velocity * deltaTime;
        
        
        if (position.y + radius > 1.0f) {
            position.y = 1.0f - radius;
            velocity.y = -velocity.y;
            hitWall = true;
            lastImpactPos = glm::vec3(position.x, 1.0f, 0.0f);
        } else if (position.y - radius < -1.0f) {
            position.y = -1.0f + radius;
            velocity.y = -velocity.y;
            hitWall = true;
            lastImpactPos = glm::vec3(position.x, -1.0f, 0.0f);
        }
        
        handlePaddleCollision(paddle1, paddle2, deltaTime);
        handlePaddleCollision(paddle2, paddle1, deltaTime);
        
        
        if (position.x - radius < -2.5f) {
            score2++;
            resetBall();
            return;
        }
        if (position.x + radius > 2.5f) {
            score1++;
            resetBall();
        }
    }
    
private:
    float clamp(float value, float min, float max) {
        return std::max(min, std::min(value, max));
    }
    
    void handlePaddleCollision(Paddle &paddle, Paddle &otherPaddle, float deltaTime) {
        float paddleLeft = paddle.position.x - paddle.size.x / 2.0f;
        float paddleRight = paddle.position.x + paddle.size.x / 2.0f;
        float paddleTop = paddle.position.y + paddle.size.y / 2.0f;
        float paddleBottom = paddle.position.y - paddle.size.y / 2.0f;
        
        float closestX = clamp(position.x, paddleLeft, paddleRight);
        float closestY = clamp(position.y, paddleBottom, paddleTop);
        
        float distanceX = position.x - closestX;
        float distanceY = position.y - closestY;
        float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
        
        if (distanceSquared < (radius * radius)) {
            float distance = std::sqrt(distanceSquared);
            if (distance == 0.0f) {
                distance = 0.001f;
            }
            
            float nx = distanceX / distance;
            float ny = distanceY / distance;
            glm::vec3 normal(nx, ny, 0.0f);
            
            velocity = glm::reflect(velocity, normal);
            position += normal * (radius - distance);
            
            float impactY = position.y - paddle.position.y;
            velocity.y += impactY * 5.0f;
            
            float maxVerticalComponent = speed * 0.75f;
            if (std::abs(velocity.y) > maxVerticalComponent) {
                velocity.y = (velocity.y > 0) ? maxVerticalComponent : -maxVerticalComponent;
            }
            
            velocity = glm::normalize(velocity) * speed;
            paddle.startRotation(360.0f);
            
            if (paddle.position.x < 0) {
                hitPaddle1 = true;
                lastImpactPos = glm::vec3(paddleRight, position.y, 0.0f);
            } else {
                hitPaddle2 = true;
                lastImpactPos = glm::vec3(paddleLeft, position.y, 0.0f);
            }
        }
    }
};


struct PongPushConstants {
    glm::mat4 model;
    glm::vec4 color;
};


struct PongUBO {
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 params;
};

class PongWindow : public mx::VKWindow {

    void spawnBurst(glm::vec3 impactPos, glm::vec3 normal, glm::vec4 paddleColor) {
        for (int i = 0; i < 35 && particles.size() < static_cast<size_t>(MAX_PARTICLES); i++) {
            Particle p;
            p.position = impactPos;
            p.velocity = normal * static_cast<float>(rand() % 50 / 10.0f + 0.5f) + 
                        glm::vec3(0, (rand() % 60 - 30) / 30.0f, (rand() % 40 - 20) / 40.0f);
            p.life = 0.6f;
            p.color = paddleColor; 
            particles.push_back(p);
        }
    }

public:
    Paddle paddle1, paddle2;
    Ball ball;
    int score1 = 0, score2 = 0;
    float gridRotation = 0.0f;
    float gridYRotation = 0.0f;
    float rotationSpeed = 50.0f;
    Uint64 lastFrameTime = 0;
    
    bool mouseDragging = false;
    int lastMouseX = 0;
    int lastMouseY = 0;
    float mouseSensitivity = 0.5f;
    
    VkPipeline pongPipeline = VK_NULL_HANDLE;
    VkPipeline pongPipelineWireframe = VK_NULL_HANDLE;
    VkPipelineLayout pongPipelineLayout = VK_NULL_HANDLE;
    
    VkImage ballTextureImage = VK_NULL_HANDLE;
    VkDeviceMemory ballTextureImageMemory = VK_NULL_HANDLE;
    VkImageView ballTextureImageView = VK_NULL_HANDLE;
        
    PongWindow(const std::string& path, int wx, int wy, bool full) 
        : mx::VKWindow("-[ VK Pong ]-", wx, wy, full),
          paddle1(glm::vec3(-0.9f, 0.0f, 0.0f), glm::vec3(0.1f, 0.4f, 0.1f)),
          paddle2(glm::vec3(0.9f, 0.0f, 0.0f), glm::vec3(0.1f, 0.4f, 0.1f)),
          ball(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.3f, 0.0f), 0.05f) {
        setPath(path);
        cameraZ = 5.0f;
        cameraX = 0.0f;
        cameraY = 0.0f;
        cameraYaw = -90.0f;
        cameraPitch = 0.0f;
        cameraSpeed = 5.0f;
        lastFrameTime = SDL_GetPerformanceCounter();
        ball.resetBall();
    }
    
    virtual ~PongWindow() {}
    
    void cleanup() override {
        if (device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device);
            
            if (ballTextureImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device, ballTextureImageView, nullptr);
                ballTextureImageView = VK_NULL_HANDLE;
            }
            if (ballTextureImage != VK_NULL_HANDLE) {
                vkDestroyImage(device, ballTextureImage, nullptr);
                vkFreeMemory(device, ballTextureImageMemory, nullptr);
                ballTextureImage = VK_NULL_HANDLE;
            }
            
            if (pongPipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(device, pongPipeline, nullptr);
                pongPipeline = VK_NULL_HANDLE;
            }
            if (pongPipelineWireframe != VK_NULL_HANDLE) {
                vkDestroyPipeline(device, pongPipelineWireframe, nullptr);
                pongPipelineWireframe = VK_NULL_HANDLE;
            }
            if (pongPipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(device, pongPipelineLayout, nullptr);
                pongPipelineLayout = VK_NULL_HANDLE;
            }
        }
        mx::VKWindow::cleanup();
    }
    
    void initVulkan() override {
        mx::VKWindow::initVulkan();
        loadCubeVertexBuffer();
        createPongPipeline();
        createParticlePipeline();  
        initStarfield(30000);
        loadBallTexture();
    }
    
    void loadBallTexture() {
        SDL_Surface* ballSurface = png::LoadPNG(util.getFilePath("ball.png").c_str());
        if (!ballSurface) {
            std::cerr << "Warning: Failed to load ball.png, using bg.png for ball" << std::endl;
            return;
        }
        
        VkDeviceSize imageSize = ballSurface->w * ballSurface->h * 4;
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);
        
        void* data;
        VK_CHECK_RESULT(vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data));
        memcpy(data, ballSurface->pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);
        
        createImage(ballSurface->w, ballSurface->h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ballTextureImage, ballTextureImageMemory);
        
        transitionImageLayout(ballTextureImage, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, ballTextureImage, ballSurface->w, ballSurface->h);
        transitionImageLayout(ballTextureImage, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
        
        
        ballTextureImageView = createImageView(ballTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
        
        SDL_FreeSurface(ballSurface);
        std::cout << ">> [BallTexture] Loaded ball.png texture\n";
    }
    
    void loadCubeVertexBuffer() {
        
        vkDeviceWaitIdle(device);
        
        
        if (vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, vertexBuffer, nullptr);
            vkFreeMemory(device, vertexBufferMemory, nullptr);
            vertexBuffer = VK_NULL_HANDLE;
            vertexBufferMemory = VK_NULL_HANDLE;
        }
        if (indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, indexBuffer, nullptr);
            vkFreeMemory(device, indexBufferMemory, nullptr);
            indexBuffer = VK_NULL_HANDLE;
            indexBufferMemory = VK_NULL_HANDLE;
        }
        
        
        std::vector<mx::Vertex> vertices;
        std::vector<uint32_t> indices;
        
        std::string cubePath = util.getFilePath("cube.mxmod");
        std::ifstream f(cubePath);
        if (!f.is_open()) {
            throw mx::Exception("Failed to load cube.mxmod");
        }
            
        std::string triTag;
        int a = 0, b = 0;
        f >> triTag >> a >> b;
        
        std::string vertTag;
        int vcount = 0;
        f >> vertTag >> vcount;
        
        std::vector<glm::vec3> positions(vcount);
        for (int i = 0; i < vcount; ++i) {
            f >> positions[i].x >> positions[i].y >> positions[i].z;
        }
        
        std::string texTag;
        int tcount = 0;
        f >> texTag >> tcount;
        
        std::vector<glm::vec2> texCoords(vcount);
        for (int i = 0; i < vcount; ++i) {
            f >> texCoords[i].x >> texCoords[i].y;
        }
        
        std::string normTag;
        int ncount = 0;
        f >> normTag >> ncount;
        
        std::vector<glm::vec3> normals(vcount);
        for (int i = 0; i < vcount; ++i) {
            f >> normals[i].x >> normals[i].y >> normals[i].z;
        }
        
        f.close();
        
        for (int i = 0; i < vcount; ++i) {
            mx::Vertex v{};
            v.pos[0] = positions[i].x;
            v.pos[1] = positions[i].y;
            v.pos[2] = positions[i].z;
            v.texCoord[0] = texCoords[i].x;
            v.texCoord[1] = texCoords[i].y;
            v.normal[0] = normals[i].x;
            v.normal[1] = normals[i].y;
            v.normal[2] = normals[i].z;
            vertices.push_back(v);
            indices.push_back(i);
        }
        
        VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
        VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
        
        
        VkBuffer stagingVertexBuffer;
        VkDeviceMemory stagingVertexBufferMemory;
        createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingVertexBuffer, stagingVertexBufferMemory);
        
        VkBuffer stagingIndexBuffer;
        VkDeviceMemory stagingIndexBufferMemory;
        createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingIndexBuffer, stagingIndexBufferMemory);
        
        void* vertexData = nullptr;
        VK_CHECK_RESULT(vkMapMemory(device, stagingVertexBufferMemory, 0, vertexBufferSize, 0, &vertexData));
        memcpy(vertexData, vertices.data(), (size_t)vertexBufferSize);
        vkUnmapMemory(device, stagingVertexBufferMemory);
        
        void* indexData = nullptr;
        VK_CHECK_RESULT(vkMapMemory(device, stagingIndexBufferMemory, 0, indexBufferSize, 0, &indexData));
        memcpy(indexData, indices.data(), (size_t)indexBufferSize);
        vkUnmapMemory(device, stagingIndexBufferMemory);
        
        createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
        
        createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
        
        copyBuffer(stagingVertexBuffer, vertexBuffer, vertexBufferSize);
        copyBuffer(stagingIndexBuffer, indexBuffer, indexBufferSize);
        
        vkDestroyBuffer(device, stagingVertexBuffer, nullptr);
        vkFreeMemory(device, stagingVertexBufferMemory, nullptr);
        vkDestroyBuffer(device, stagingIndexBuffer, nullptr);
        vkFreeMemory(device, stagingIndexBufferMemory, nullptr);
        
        indexCount = static_cast<uint32_t>(indices.size());
        
        std::cout << ">> [PongVertexBuffer] Loaded cube model with " << vcount << " vertices\n";
    }
    
    void createPongPipeline() {
        
        auto vertShaderCode = mx::readFile(util.getFilePath("pong_vert.spv"));
        auto fragShaderCode = mx::readFile(util.getFilePath("pong_frag.spv"));
        
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
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
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
        depthStencil.depthWriteEnable = VK_TRUE;
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
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PongPushConstants);
        
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pongPipelineLayout) != VK_SUCCESS) {
            throw mx::Exception("Failed to create pong pipeline layout!");
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
        pipelineInfo.layout = pongPipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pongPipeline) != VK_SUCCESS) {
            throw mx::Exception("Failed to create pong graphics pipeline!");
        }
        
        
        rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pongPipelineWireframe) != VK_SUCCESS) {
            throw mx::Exception("Failed to create pong wireframe pipeline!");
        }
        
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        
        std::cout << ">> [PongPipeline] Created with push constants for model/color\n";
    }
    
    virtual void event(SDL_Event& e) override {
        if (e.type == SDL_QUIT) {
            quit();
            return;
        }
        
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_SPACE:
                    if (currentPolygonMode == VK_POLYGON_MODE_FILL) {
                        currentPolygonMode = VK_POLYGON_MODE_LINE;
                    } else {
                        currentPolygonMode = VK_POLYGON_MODE_FILL;
                    }
                    break;
                case SDLK_RETURN:
                    cameraZ = 5.0f;
                    cameraX = 0.0f;
                    cameraY = 0.0f;
                    cameraYaw = -90.0f;
                    cameraPitch = 0.0f;
                    gridRotation = 0.0f;
                    gridYRotation = 0.0f;
                    break;
                case SDLK_ESCAPE:
                    quit();
                    break;
                case SDLK_r:
                    score1 = 0;
                    score2 = 0;
                    ball.resetBall();
                    paddle1.position.y = 0.0f;
                    paddle2.position.y = 0.0f;
                    break;
            }
        }
        
        
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (e.button.button == SDL_BUTTON_LEFT || e.button.button == SDL_BUTTON_RIGHT) {
                mouseDragging = true;
                lastMouseX = e.button.x;
                lastMouseY = e.button.y;
            }
        }
        
        if (e.type == SDL_MOUSEBUTTONUP) {
            if (e.button.button == SDL_BUTTON_LEFT || e.button.button == SDL_BUTTON_RIGHT) {
                mouseDragging = false;
            }
        }
        
        if (e.type == SDL_MOUSEMOTION) {
            if (mouseDragging) {
                int deltaX = e.motion.x - lastMouseX;
                int deltaY = e.motion.y - lastMouseY;
                
                gridYRotation += deltaX * mouseSensitivity;
                gridRotation += deltaY * mouseSensitivity;
                
                if (gridRotation > 89.0f) gridRotation = 89.0f;
                if (gridRotation < -89.0f) gridRotation = -89.0f;
                
                lastMouseX = e.motion.x;
                lastMouseY = e.motion.y;
            } else {
                int mouseY = e.motion.y;
                float normalizedY = (mouseY / (float)h) * 2.0f - 1.0f;
                paddle1.position.y = -normalizedY;
                clampPaddle(paddle1);
            }
        }
        
        if (e.type == SDL_MOUSEWHEEL) {
            cameraZ -= e.wheel.y * 0.5f;
            if (cameraZ < 1.0f) cameraZ = 1.0f;
            if (cameraZ > 20.0f) cameraZ = 20.0f;
        }
        
        
        if (e.type == SDL_FINGERMOTION) {
            float touchY = e.tfinger.y;
            float normalizedY = touchY * 2.0f - 1.0f;
            paddle1.position.y = -normalizedY;
            clampPaddle(paddle1);
        }
    }
    
    void clampPaddle(Paddle &paddle) {
        float halfPaddleHeight = paddle.size.y / 2.0f;
        if (paddle.position.y + halfPaddleHeight > 1.0f) {
            paddle.position.y = 1.0f - halfPaddleHeight;
        } else if (paddle.position.y - halfPaddleHeight < -1.0f) {
            paddle.position.y = -1.0f + halfPaddleHeight;
        }
    }

    void updateParticles(float deltaTime) {
        if (mappedParticleData == nullptr) return;
        
        glm::vec3 gravity(0.0f, -2.0f, 0.0f);
        activeParticleCount = 0;
        ParticleVertex* particleBufferData = static_cast<ParticleVertex*>(mappedParticleData);
        
        particles.erase(
            std::remove_if(particles.begin(), particles.end(), 
                [](const Particle& p) { return p.life <= 0.0f; }),
            particles.end()
        );
        
        for (auto& p : particles) {
            if (p.life > 0.0f && activeParticleCount < static_cast<uint32_t>(MAX_PARTICLES)) {
                p.velocity += gravity * deltaTime;
                p.position += p.velocity * deltaTime;
                p.life -= deltaTime * 1.5f;
                p.color.a = p.life;
                particleBufferData[activeParticleCount] = { p.position, p.color };
                activeParticleCount++;
            }
        }
    }

    
    virtual void proc() override {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float deltaTime = (currentTime - lastFrameTime) / (double)SDL_GetPerformanceFrequency();
        lastFrameTime = currentTime;
        
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        const Uint8* keyState = SDL_GetKeyboardState(nullptr);
        
        
        if (keyState[SDL_SCANCODE_A]) {
            gridRotation -= rotationSpeed * deltaTime;
        }
        if (keyState[SDL_SCANCODE_D]) {
            gridRotation += rotationSpeed * deltaTime;
        }
        if (keyState[SDL_SCANCODE_S]) {
            gridYRotation -= rotationSpeed * deltaTime;
        }
        if (keyState[SDL_SCANCODE_W]) {
            gridYRotation += rotationSpeed * deltaTime;
        }
        if (keyState[SDL_SCANCODE_Q]) {
            gridRotation = 0;
            gridYRotation = 0;
        }
        
        
        if (gridRotation >= 360.0f) gridRotation -= 360.0f;
        if (gridRotation <= -360.0f) gridRotation += 360.0f;
        if (gridYRotation >= 360.0f) gridYRotation -= 360.0f;
        if (gridYRotation <= -360.0f) gridYRotation += 360.0f;
        
        
        float speed = 2.0f;
        if (keyState[SDL_SCANCODE_UP] && paddle1.position.y + paddle1.size.y / 2 < 1.0f) {
            paddle1.position.y += speed * deltaTime;
        }
        if (keyState[SDL_SCANCODE_DOWN] && paddle1.position.y - paddle1.size.y / 2 > -1.0f) {
            paddle1.position.y -= speed * deltaTime;
        }
        
        
        float paddleSpeed = 0.015f;
        if (ball.position.y > paddle2.position.y + paddle2.size.y / 4 &&
            paddle2.position.y + paddle2.size.y / 2 < 1.0f) {
            paddle2.position.y += paddleSpeed;
        }
        if (ball.position.y < paddle2.position.y - paddle2.size.y / 4 &&
            paddle2.position.y - paddle2.size.y / 2 > -1.0f) {
            paddle2.position.y -= paddleSpeed;
        }
        
        
        paddle1.update(deltaTime);
        paddle2.update(deltaTime);
        ball.update(deltaTime, paddle1, paddle2, score1, score2);
        
        
        if (ball.hitPaddle1) {
            spawnBurst(ball.lastImpactPos, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec4(0.3f, 0.6f, 1.0f, 1.0f));
        }
        if (ball.hitPaddle2) {
            spawnBurst(ball.lastImpactPos, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.3f, 0.3f, 1.0f));
        }
        
        
        updateParticles(deltaTime);
       
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
        
        std::ostringstream scoreStream;
        scoreStream << "Player 1: " << score1 << " : Player 2: " << score2;
        std::string scoreText = scoreStream.str();
        std::ostringstream fpsStream;
        fpsStream << std::fixed << std::setprecision(1) << "FPS: " << fps;
        std::string fpsText = fpsStream.str();
        std::string polygonMode = (currentPolygonMode == VK_POLYGON_MODE_LINE) ? "WIREFRAME" : "SOLID";
        printText("Vulkan Pong", 50, 50, white);
        printText(scoreText, 50, 80, yellow);
        printText(fpsText + " | Mode: " + polygonMode, 50, 110, white);
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
        
        
        drawStarfield(commandBuffers[imageIndex], imageIndex);

        VkPipeline pipelineToUse = (currentPolygonMode == VK_POLYGON_MODE_LINE) ? pongPipelineWireframe : pongPipeline;
        vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineToUse);

        if (vertexBuffer != VK_NULL_HANDLE) {
            VkBuffer vertexBuffers[] = { vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);
        }

        if (indexBuffer != VK_NULL_HANDLE) {
            vkCmdBindIndexBuffer(commandBuffers[imageIndex], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }


        
        glm::mat4 view = glm::mat4(1.0f);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f)); 
        view = glm::rotate(view, glm::radians(gridRotation), glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, glm::radians(gridYRotation), glm::vec3(0.0f, 1.0f, 0.0f));
        
        
        float zNear = 0.1f;
        float zFar = 100.0f;
        float aspectRatio = static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspectRatio, zNear, zFar);
        proj[1][1] *= -1; 
        
        {
            mx::UniformBufferObject ubo{};
            ubo.model = glm::mat4(1.0f);  
            ubo.view = view;
            ubo.proj = proj;
            ubo.color = glm::vec4(1.0f);
            ubo.params = glm::vec4(0.0f);
            
            if (uniformBuffersMapped.size() > imageIndex && uniformBuffersMapped[imageIndex] != nullptr) {
                memcpy(uniformBuffersMapped[imageIndex], &ubo, sizeof(ubo));
            }
        }
        
        if (!descriptorSets.empty()) {
            vkCmdBindDescriptorSets(
                commandBuffers[imageIndex],
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pongPipelineLayout,
                0,                              
                1,                              
                &descriptorSets[imageIndex],    
                0,                              
                nullptr                         
            );
        }

        
        {
            PongPushConstants pc{};
            pc.model = paddle1.getModelMatrix();
            pc.color = glm::vec4(0.3f, 0.6f, 1.0f, 1.0f); 
            
            vkCmdPushConstants(commandBuffers[imageIndex], pongPipelineLayout, 
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                0, sizeof(PongPushConstants), &pc);
            vkCmdDrawIndexed(commandBuffers[imageIndex], indexCount, 1, 0, 0, 0);
        }

        
        {
            PongPushConstants pc{};
            pc.model = paddle2.getModelMatrix();
            pc.color = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f); 
            
            vkCmdPushConstants(commandBuffers[imageIndex], pongPipelineLayout, 
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                0, sizeof(PongPushConstants), &pc);
            vkCmdDrawIndexed(commandBuffers[imageIndex], indexCount, 1, 0, 0, 0);
        }

        
        {
            PongPushConstants pc{};
            pc.model = ball.getModelMatrix();
            pc.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); 
            
            if (ballTextureImageView != VK_NULL_HANDLE) {
                VkDescriptorImageInfo ballImageInfo{};
                ballImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                ballImageInfo.imageView = ballTextureImageView;
                ballImageInfo.sampler = textureSampler;
                
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = uniformBuffers[imageIndex];
                bufferInfo.offset = 0;
                bufferInfo.range = sizeof(mx::UniformBufferObject);
                
                std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstSet = descriptorSets[imageIndex];
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[0].descriptorCount = 1;
                descriptorWrites[0].pImageInfo = &ballImageInfo;
                
                descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[1].dstSet = descriptorSets[imageIndex];
                descriptorWrites[1].dstBinding = 1;
                descriptorWrites[1].dstArrayElement = 0;
                descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[1].descriptorCount = 1;
                descriptorWrites[1].pBufferInfo = &bufferInfo;
                
                vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), 
                                       descriptorWrites.data(), 0, nullptr);
            }
            
            vkCmdPushConstants(commandBuffers[imageIndex], pongPipelineLayout, 
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                0, sizeof(PongPushConstants), &pc);
            vkCmdDrawIndexed(commandBuffers[imageIndex], indexCount, 1, 0, 0, 0);
            
            if (ballTextureImageView != VK_NULL_HANDLE) {
                VkDescriptorImageInfo paddleImageInfo{};
                paddleImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                paddleImageInfo.imageView = textureImageView;
                paddleImageInfo.sampler = textureSampler;
                
                VkWriteDescriptorSet restoreWrite{};
                restoreWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                restoreWrite.dstSet = descriptorSets[imageIndex];
                restoreWrite.dstBinding = 0;
                restoreWrite.dstArrayElement = 0;
                restoreWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                restoreWrite.descriptorCount = 1;
                restoreWrite.pImageInfo = &paddleImageInfo;
                
                vkUpdateDescriptorSets(device, 1, &restoreWrite, 0, nullptr);
            }
        }
        
        
        drawParticles(commandBuffers[imageIndex], imageIndex);
        
        if (textRenderer && textPipeline != VK_NULL_HANDLE) {
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
        clearTextQueue();
    }

private:
};

int main(int argc, char **argv) {
#if defined(__APPLE__) || defined(_WIN32) || defined(_WIN64) || defined(__linux__)
#ifndef __ANDROID__
    Arguments args = proc_args(argc, argv);
    try {
        PongWindow window(args.path, args.width, args.height, args.fullscreen);
        window.initVulkan();
        window.loop();   
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
#endif
#elif defined(__ANDROID__)
    try {
        PongWindow window("", 960, 720, false);
        window.initVulkan();
        window.loop();   
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
#endif
    return EXIT_SUCCESS;
}
