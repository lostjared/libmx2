#ifndef MX_CROSS_H_
#define MX_CROSS_H_

#include <memory>
#include <string>
#include <SDL2/SDL.h>
#include "config.h"
#include "exception.hpp"

class MX_WindowBase;
class MX_App;

class MX_App {
public:
    virtual ~MX_App() = default;
    virtual void load(MX_WindowBase *win) {}
    virtual void draw(MX_WindowBase *win) {}
    virtual void resize(MX_WindowBase *win, int w, int h) {}
    virtual void event(MX_WindowBase *win, SDL_Event &e) {}
    virtual void cleanup(MX_WindowBase *win) {}
};

class MX_WindowBase {
public:
    virtual ~MX_WindowBase() = default;
    virtual void run(const std::string &title, int w, int h, bool fullscreen = false) = 0;
    virtual void loop() = 0;
    virtual void quit() = 0;
    virtual void cleanup() = 0;
    virtual void setApp(MX_App *a) = 0;
    virtual void setPath(const std::string &path) = 0;
    virtual void event(SDL_Event &e) = 0;
    virtual void draw() = 0;
    virtual void drawTriangle() {}
};

#if defined(WITH_GL)
#include "gl.hpp"

class MX_GLAdapter : public MX_WindowBase {
public:
    MX_GLAdapter() = default;
    ~MX_GLAdapter() override { cleanup(); }

    void run(const std::string &title, int w, int h, bool fullscreen = false) override {
        window = std::make_unique<Window>(title, w, h, this);
        if (!pending_path.empty()) window->setPath(pending_path);
        if (app) app->load(this);
        window->loop();
    }
    void loop() override { if (window) window->loop(); }
    void quit() override { if (window) window->quit(); }
    void cleanup() override { if (app) app->cleanup(this); window.reset(); }
    void setApp(MX_App *a) override { app = a; }
    void setPath(const std::string &path) override { pending_path = path; if (window) window->setPath(path); }
    void event(SDL_Event &e) override { if (app) app->event(this, e); }
    void draw() override { if (window) window->draw(); }

    void drawTriangle() override;

private:
    class Bridge;
    class Window : public gl::GLWindow {
    public:
        Window(const std::string &title, int w, int h, MX_GLAdapter *owner)
            : gl::GLWindow(title, w, h, true), owner(owner) {
            setObject(new Bridge(owner));
        }
        void event(SDL_Event &e) override { if (owner) owner->event(e); }
        void draw() override {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            if (owner) owner->drawTriangle();
            if (owner && owner->app) owner->app->draw(owner);
            swap();
        }
    private:
        MX_GLAdapter *owner;
    };

    class Bridge : public gl::GLObject {
    public:
        explicit Bridge(MX_GLAdapter *owner) : owner(owner) {}
        void load(gl::GLWindow *win) override { if (owner && owner->app) owner->app->load(owner); }
        void draw(gl::GLWindow *win) override { if (owner && owner->app) owner->app->draw(owner); }
        void event(gl::GLWindow *win, SDL_Event &e) override { if (owner && owner->app) owner->app->event(owner, e); }
        void resize(gl::GLWindow *win, int w, int h) override { if (owner && owner->app) owner->app->resize(owner, w, h); }
    private:
        MX_GLAdapter *owner;
    };

    std::unique_ptr<Window> window;
    MX_App *app = nullptr;
    std::string pending_path;
    unsigned int triProgram = 0;
    unsigned int triVao = 0;
    unsigned int triVbo = 0;
    bool triReady = false;
    int triModelLoc = -1;
    int triViewLoc = -1;
    int triProjLoc = -1;
    int triParamsLoc = -1;
    int triColorLoc = -1;
};

inline void MX_GLAdapter::drawTriangle() {
    if (!window) return;
    if (!triReady) {
        const char* vsrc = R"glsl(#version 410 core
            layout(location = 0) in vec3 inPosition;
            layout(location = 1) in vec2 inTexCoord;
            layout(location = 2) in vec3 inNormal;
            layout(location = 1) out vec2 fragTexCoord;
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 proj;
            void main() {
                fragTexCoord = inTexCoord;
                gl_Position = proj * view * model * vec4(inPosition, 1.0);
            }
            )glsl";

        const char* fsrc = R"glsl(#version 410 core
            layout(location = 1) in vec2 fragTexCoord;
            layout(location = 0) out vec4 outColor;
            uniform vec4 color;
            uniform vec4 params;
            void main() {
                float u = fragTexCoord.x;
                float v = fragTexCoord.y;
                float t = u * 6.2831853;
                float r = sin(t + v * 3.14159 + params.x);
                float g = sin(t * 0.8 + v * 2.0 + 2.0 + params.x);
                float b = sin(t * 1.2 + v * 1.5 + 4.0 + params.x);
                vec3 col = 0.5 + 0.5 * vec3(r, g, b);
                outColor = vec4(col * color.xyz, color.w);
            }
            )glsl";

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vsrc, nullptr);
        glCompileShader(vs);
        GLint compiled = 0;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint len = 0; glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &len);
            std::string log; log.resize(len>0?len:1);
            glGetShaderInfoLog(vs, len, nullptr, &log[0]);
            SDL_Log("GL vertex shader compile failed: %s", log.c_str());
            glDeleteShader(vs);
            return;
        }

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fsrc, nullptr);
        glCompileShader(fs);
        glGetShaderiv(fs, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint len = 0; glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &len);
            std::string log; log.resize(len>0?len:1);
            glGetShaderInfoLog(fs, len, nullptr, &log[0]);
            SDL_Log("GL fragment shader compile failed: %s", log.c_str());
            glDeleteShader(vs); glDeleteShader(fs);
            return;
        }

        triProgram = glCreateProgram();
        glAttachShader(triProgram, vs);
        glAttachShader(triProgram, fs);
        glLinkProgram(triProgram);
        GLint linked = 0; glGetProgramiv(triProgram, GL_LINK_STATUS, &linked);
        if (!linked) {
            GLint len = 0; glGetProgramiv(triProgram, GL_INFO_LOG_LENGTH, &len);
            std::string log; log.resize(len>0?len:1);
            glGetProgramInfoLog(triProgram, len, nullptr, &log[0]);
            SDL_Log("GL program link failed: %s", log.c_str());
            glDeleteProgram(triProgram);
            triProgram = 0;
            glDeleteShader(vs); glDeleteShader(fs);
            return;
        }
        glDeleteShader(vs);
        glDeleteShader(fs);

        float verts[] = {
            -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,   0.0f,0.0f,1.0f,
             0.5f, -0.5f, 0.0f,  1.0f, 0.0f,   0.0f,0.0f,1.0f,
             0.0f,  0.5f, 0.0f,  0.5f, 1.0f,   0.0f,0.0f,1.0f
        };

        glGenVertexArrays(1, &triVao);
        glGenBuffers(1, &triVbo);
        glBindVertexArray(triVao);
        glBindBuffer(GL_ARRAY_BUFFER, triVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        GLsizei stride = 8 * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            
            triModelLoc = glGetUniformLocation(triProgram, "model");
            triViewLoc = glGetUniformLocation(triProgram, "view");
            triProjLoc = glGetUniformLocation(triProgram, "proj");
            triParamsLoc = glGetUniformLocation(triProgram, "params");
            triColorLoc = glGetUniformLocation(triProgram, "color");
            triReady = true;
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(triProgram);

    
    float time = SDL_GetTicks() / 1000.0f;
    if (triModelLoc >= 0) {
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(1.0f,1.0f,1.0f));
        glUniformMatrix4fv(triModelLoc, 1, GL_FALSE, &model[0][0]);
    }
    if (triViewLoc >= 0) {
        glm::vec3 cameraPos(0.0f,0.0f,3.0f);
        glm::vec3 cameraTarget(0.0f,0.0f,0.0f);
        glm::vec3 up(0.0f,1.0f,0.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, up);
        glUniformMatrix4fv(triViewLoc, 1, GL_FALSE, &view[0][0]);
    }
    if (triProjLoc >= 0) {
        int w = window->w, h = window->h;
        float aspect = (h==0) ? 1.0f : (float)w / (float)h;
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
        
        proj[1][1] *= -1;
        glUniformMatrix4fv(triProjLoc, 1, GL_FALSE, &proj[0][0]);
    }
    if (triParamsLoc >= 0) {
        glUniform4f(triParamsLoc, time, 0.0f, 0.0f, 0.0f);
    }
    if (triColorLoc >= 0) {
        glUniform4f(triColorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    glBindVertexArray(triVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        SDL_Log("GL error after draw: 0x%X", err);
    }
}

#endif


#if (defined(WITH_VULKAN) || defined(VULKAN) || defined(USE_VULKAN) || defined(WITH_VK))
#include "vk.hpp"

class MX_VKAdapter : public MX_WindowBase, public mx::VKWindow {
public:
    MX_VKAdapter() = default;
    ~MX_VKAdapter() override { cleanup(); }

    void run(const std::string &title, int w, int h, bool fullscreen = false) override {
        initWindow(title, w, h, fullscreen);
        if (!pending_path.empty()) mx::VKWindow::setPath(pending_path);
        initVulkan();
        if (app) app->load(this);
        loop();
    }

    void loop() override { mx::VKWindow::loop(); }
    void quit() override { mx::VKWindow::quit(); }
    void cleanup() override { if (app) app->cleanup(this); mx::VKWindow::cleanup(); }
    void setApp(MX_App *a) override { app = a; }
    void setPath(const std::string &path) override { pending_path = path; mx::VKWindow::setPath(path); }

    void event(SDL_Event &e) override { if (app) app->event(this, e); }
    void draw() override { 
        this->drawTriangle();
        this->currentPolygonMode = VK_POLYGON_MODE_FILL;
        mx::VKWindow::draw(); 
        if (app) app->draw(this); 
    }
    void drawTriangle() override {
        if (vertexBuffer != VK_NULL_HANDLE && indexBuffer != VK_NULL_HANDLE && indexCount > 0) return;
        std::vector<mx::Vertex> verts = {
            mx::Vertex{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
            mx::Vertex{ {  0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
            mx::Vertex{ {  0.0f,  0.5f, 0.0f }, { 0.5f, 1.0f }, { 0.0f, 0.0f, 1.0f } }
        };
        std::vector<uint32_t> inds = {0,1,2};

        VkDeviceSize vertexSize = sizeof(mx::Vertex) * verts.size();
        VkDeviceSize indexSize = sizeof(uint32_t) * inds.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);
        void* data;
        VK_CHECK_RESULT(vkMapMemory(device, stagingBufferMemory, 0, vertexSize, 0, &data));
        memcpy(data, verts.data(), static_cast<size_t>(vertexSize));
        vkUnmapMemory(device, stagingBufferMemory);

        if (vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, vertexBuffer, nullptr);
            vkFreeMemory(device, vertexBufferMemory, nullptr);
        }
        createBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
        copyBuffer(stagingBuffer, vertexBuffer, vertexSize);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

        createBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);
        VK_CHECK_RESULT(vkMapMemory(device, stagingBufferMemory, 0, indexSize, 0, &data));
        memcpy(data, inds.data(), static_cast<size_t>(indexSize));
        vkUnmapMemory(device, stagingBufferMemory);

        if (indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, indexBuffer, nullptr);
            vkFreeMemory(device, indexBufferMemory, nullptr);
        }
        createBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
        copyBuffer(stagingBuffer, indexBuffer, indexSize);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

        indexCount = static_cast<uint32_t>(inds.size());
        (void)vertexBuffer; (void)indexBuffer; (void)indexCount;
        if (textureImage == VK_NULL_HANDLE) {
            setupTextureImage(1,1);
            uint8_t whitePixel[4] = {255,255,255,255};
            updateTexture(whitePixel, 4);
            createTextureImageView();
            createTextureSampler();
            try { updateDescriptorSets(); } catch (...) { }
        }
    }

private:
    MX_App *app = nullptr;
    std::string pending_path;
};
#endif

inline std::unique_ptr<MX_WindowBase> create_gl_window() {
#if defined(WITH_GL)
    return std::make_unique<MX_GLAdapter>();
#else
    throw mx::Exception("GL support not compiled in");
#endif
}

#if defined(WITH_VULKAN) || defined(VULKAN) || defined(USE_VULKAN) || defined(WITH_VK)
inline std::unique_ptr<MX_WindowBase> create_vk_window() {
    return std::make_unique<MX_VKAdapter>();
}
#endif

#endif  