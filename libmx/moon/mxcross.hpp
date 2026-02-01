#ifndef MXCROSS_H_
#define MXCROSS_H_
#ifndef __EMSCRIPTEN__
#include"config.h"
#endif
#include"mx.hpp"
#include"argz.hpp"
#include"loadpng.hpp"
#include<fstream>
#include<sstream>
#include<vector>
#include<memory>

#ifdef __EMSCRIPTEN__
#include"glm.hpp"
#else
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#endif

#ifdef WITH_GL
#include"gl.hpp"
#endif
#ifdef WITH_VK
#include"vk.hpp"
#endif

namespace cross {

    struct ModelVertex {
        float pos[3];
        float texCoord[2];
        float normal[3];
    };

    struct Vec2 { float x, y; };
    struct Vec3 { float x, y, z; };

    inline void loadMXMOD(
        const std::string& path,
        std::vector<ModelVertex>& outVertices,
        std::vector<uint32_t>& outIndices,
        float positionScale = 1.0f
    ) {
        std::ifstream f(path);
        if (!f.is_open()) {
            throw mx::Exception("loadMXMOD: failed to open file: " + path);
        }

        auto readHeader = [&](const char* expectedTag, int& countOut) {
            std::string line;
            while (std::getline(f, line)) {
                size_t p = line.find_first_not_of(" \t\r\n");
                if (p == std::string::npos) continue;
                if (line[p] == '#') continue;
                
                std::istringstream iss(line);
                std::string tag;
                if (!(iss >> tag >> countOut)) {
                    throw mx::Exception(std::string("loadMXMOD: missing header: ") + expectedTag);
                }
                if (tag != expectedTag) {
                    throw mx::Exception(std::string("loadMXMOD: expected header '") + expectedTag + "', got '" + tag + "'");
                }
                return;
            }
            throw mx::Exception(std::string("loadMXMOD: missing header: ") + expectedTag);
        };

        {
            std::string line;
            while (std::getline(f, line)) {
                size_t p = line.find_first_not_of(" \t\r\n");
                if (p == std::string::npos) continue;
                if (line[p] == '#') continue;
                
                std::istringstream iss(line);
                std::string triTag;
                int a = 0, b = 0;
                if (!(iss >> triTag >> a >> b)) {
                    throw mx::Exception("loadMXMOD: missing tri header");
                }
                if (triTag != "tri") {
                    throw mx::Exception("loadMXMOD: expected 'tri' header");
                }
                break;
            }
        }

        auto readNonCommentLine = [&](std::string &line) -> bool {
            while (std::getline(f, line)) {
                size_t p = line.find_first_not_of(" \t\r\n");
                if (p == std::string::npos) continue;
                if (line[p] == '#') continue;
                return true;
            }
            return false;
        };

        int vcount = 0;
        readHeader("vert", vcount);
        if (vcount <= 0) throw mx::Exception("loadMXMOD: invalid vert count");

        std::vector<Vec3> pos(vcount);
        std::string line;
        int read = 0;
        while (read < vcount) {
            if (!readNonCommentLine(line)) throw mx::Exception("loadMXMOD: failed reading vertex positions");
            std::istringstream iss(line);
            if (!(iss >> pos[read].x >> pos[read].y >> pos[read].z)) {
                throw mx::Exception("loadMXMOD: failed reading vertex positions");
            }
            pos[read].x *= positionScale;
            pos[read].y *= positionScale;
            pos[read].z *= positionScale;
            ++read;
        }

        int tcount = 0;
        readHeader("tex", tcount);
        if (tcount != vcount) throw mx::Exception("loadMXMOD: tex count != vert count");

        std::vector<Vec2> uv(vcount);
        read = 0;
        while (read < vcount) {
            if (!readNonCommentLine(line)) throw mx::Exception("loadMXMOD: failed reading texcoords");
            std::istringstream iss(line);
            if (!(iss >> uv[read].x >> uv[read].y)) throw mx::Exception("loadMXMOD: failed reading texcoords");
            ++read;
        }

        int ncount = 0;
        readHeader("norm", ncount);
        if (ncount != vcount) throw mx::Exception("loadMXMOD: norm count != vert count");

        std::vector<Vec3> nrm(vcount);
        read = 0;
        while (read < vcount) {
            if (!readNonCommentLine(line)) throw mx::Exception("loadMXMOD: failed reading normals");
            std::istringstream iss(line);
            if (!(iss >> nrm[read].x >> nrm[read].y >> nrm[read].z)) throw mx::Exception("loadMXMOD: failed reading normals");
            ++read;
        }

        outVertices.clear();
        outVertices.reserve(static_cast<size_t>(vcount));
        for (int i = 0; i < vcount; ++i) {
            ModelVertex v{};
            v.pos[0] = pos[i].x;
            v.pos[1] = pos[i].y;
            v.pos[2] = pos[i].z;
            v.texCoord[0] = uv[i].x;
            v.texCoord[1] = uv[i].y;
            v.normal[0] = nrm[i].x;
            v.normal[1] = nrm[i].y;
            v.normal[2] = nrm[i].z;
            outVertices.push_back(v);
        }

        outIndices.clear();
        outIndices.reserve(static_cast<size_t>(vcount));
        for (uint32_t i = 0; i < static_cast<uint32_t>(vcount); ++i) outIndices.push_back(i);
        SDL_Log("Loaded MXMOD: %s (%d vertices)", path.c_str(), vcount);
    }

    
    
    
    class ModelBase {
    public:
        virtual ~ModelBase() = default;
        
        
        virtual void init(const std::string& dataPath, const std::string& modelFile, 
                         const std::string& textureFile, float scale = 1.0f, 
                         const glm::vec3& position = glm::vec3(0.0f, 0.0f, -5.0f)) = 0;
        
        
        virtual void update(float deltaTime) = 0;
        
        
        virtual void draw(int screenW, int screenH, const glm::vec3& cameraPos, 
                         float cameraYaw, float cameraPitch) = 0;
        
        
        virtual void cleanup() = 0;
        
        
        virtual bool isInitialized() const = 0;
        
        
        virtual void setPosition(const glm::vec3& pos) = 0;
        virtual glm::vec3 getPosition() const = 0;
        virtual void setScale(float scale) = 0;
        virtual float getScale() const = 0;
        virtual void setRotation(float x, float y, float z) = 0;
        virtual void getRotation(float& x, float& y, float& z) const = 0;
    };
    
    class WindowBase;

    class EventHandler {
    public:
        virtual ~EventHandler() = default;
        virtual void load() = 0;
        virtual void draw() = 0;
        virtual void event(SDL_Event &e) = 0;
        virtual void update(float deltaTime) {}
        virtual void resize(int w, int h) {}
        virtual void cleanup() {}
        
        void setWindow(WindowBase *w) { window = w; }
        WindowBase* getWindow() const { return window; }
        
    protected:
        WindowBase *window = nullptr;
    };

    
    class WindowBase {
    public:
        virtual ~WindowBase() = default;
        virtual void init(const std::string &title, const std::string &path, int w, int h, bool full) = 0;
        virtual void loop() = 0;
        virtual void quit() = 0;
        virtual void cleanup() = 0;
        virtual int getWidth() const = 0;
        virtual int getHeight() const = 0;
        virtual void setHandler(EventHandler *handler) = 0;
        virtual mx::mxUtil& getUtil() = 0;
        virtual const std::string& getPath() const = 0;
        
        
        template<typename T>
        T* as() { return dynamic_cast<T*>(this); }
    };

#ifdef WITH_GL
    class GL_Window : public WindowBase, public gl::GLWindow {
    public:
        GL_Window() : gl::GLWindow("", 100, 100, true) {}
        
        void init(const std::string &title, const std::string &path, int w, int h, bool full) override {
            dataPath = path;
            gl::GLWindow::setPath(path);
            gl::GLWindow::setWindowTitle(title);
            gl::GLWindow::setWindowSize(w, h);
            if (full) gl::GLWindow::setFullScreen(true);
            if (handler) handler->load();
        }
        
        void loop() override {
            gl::GLWindow::loop();
        }
        
        void quit() override {
            gl::GLWindow::quit();
        }
        
        void cleanup() override {
            if (handler) handler->cleanup();
        }
        
        int getWidth() const override { return gl::GLWindow::w; }
        int getHeight() const override { return gl::GLWindow::h; }
        
        void setHandler(EventHandler *h) override { 
            handler = h;
            if (handler) handler->setWindow(this);
            gl::GLWindow::setObject(new Bridge(this));
        }
        
        mx::mxUtil& getUtil() override { return gl::GLWindow::util; }
        const std::string& getPath() const override { return dataPath; }
        
        void event(SDL_Event &e) override {
            
        }
        
        void draw() override {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            if (handler) handler->draw();
            gl::GLWindow::swap();
        }
        
        void resize(int w, int h) override {
            if (handler) handler->resize(w, h);
        }
        
    private:
        EventHandler *handler = nullptr;
        std::string dataPath;
        
        class Bridge : public gl::GLObject {
        public:
            explicit Bridge(GL_Window *owner) : owner(owner) {}
            void load(gl::GLWindow *win) override { 
                if (owner && owner->handler) owner->handler->load(); 
            }
            void draw(gl::GLWindow *win) override { 
                if (owner && owner->handler) owner->handler->draw(); 
            }
            void event(gl::GLWindow *win, SDL_Event &e) override { 
                if (owner && owner->handler) owner->handler->event(e); 
            }
            void resize(gl::GLWindow *win, int w, int h) override { 
                if (owner && owner->handler) owner->handler->resize(w, h); 
            }
        private:
            GL_Window *owner;
        };
    };   

    inline bool loadGLShaderProgram(gl::ShaderProgram& program, const std::string& vertShaderPath, const std::string& fragShaderPath) {
        try {        
            std::string vertSource = mx::readFileToString(vertShaderPath);
            std::string fragSource = mx::readFileToString(fragShaderPath);
            if (!program.loadProgramFromText(vertSource, fragSource)) {
                SDL_Log("ERROR: Failed to compile GL shaders from %s and %s", vertShaderPath.c_str(), fragShaderPath.c_str());
                return false;
            }
            SDL_Log("loadGLShaderProgram: Shaders compiled successfully");
            return true;
        } catch (const std::exception& e) {
            SDL_Log("ERROR: Exception in loadGLShaderProgram: %s", e.what());
            return false;
        }
    }
    
    class GL_Model : public ModelBase {
    public:
        gl::ShaderProgram program;
        GLuint VAO = 0;
        GLuint VBO = 0;
        GLuint EBO = 0;
        GLuint modelTexture = 0;
        
        std::vector<ModelVertex> vertices;
        std::vector<uint32_t> indices;
        
        bool initialized = false;
        float rotationX = 0.0f;
        float rotationY = 0.0f;
        float rotationZ = 0.0f;
        float modelScale = 1.0f;
        glm::vec3 modelPosition = glm::vec3(0.0f, 0.0f, -5.0f);
        
        ~GL_Model() override {
            cleanup();
        }
     
        std::string frag_shader;
        std::string vert_shader;

        void rebindVAO() {
            SDL_Log("GL_Model::rebindVAO() - Rebinding VAO with new shader program");
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            
            
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, pos));
            glEnableVertexAttribArray(0);
            
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, texCoord));
            glEnableVertexAttribArray(1);
            
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, normal));
            glEnableVertexAttribArray(2);
            
            glBindVertexArray(0);
            SDL_Log("GL_Model::rebindVAO() - VAO rebinding complete");
        }

        void setShaders(const std::string &vert, const std::string &frag) {
            vert_shader = vert;
            frag_shader = frag;

            SDL_Log("GL_Model::setShaders() - Loading vert: %s", vert_shader.c_str());
            SDL_Log("GL_Model::setShaders() - Loading frag: %s", frag_shader.c_str());
            
            if (!loadGLShaderProgram(program, vert_shader, frag_shader)) {
                SDL_Log("ERROR: Failed to load GL model shaders");
                throw mx::Exception("Failed to load GL model shaders");
            }
            
            SDL_Log("GL_Model::setShaders() - Successfully loaded shaders");
            
            
            if (initialized) {
                rebindVAO();
            }
        }
        
        void init(const std::string& dataPath, const std::string& modelFile, const std::string& textureFile,
                  float scale = 1.0f, const glm::vec3& position = glm::vec3(0.0f, 0.0f, -5.0f)) override {
            if (initialized) return;
             
            modelScale = scale;
            modelPosition = position;
            std::string modelPath = dataPath + "/data/" + modelFile;
            loadMXMOD(modelPath, vertices, indices, modelScale);
            std::string texPath = dataPath + "/data/" + textureFile;
            modelTexture = gl::loadTexture(texPath);
            if (!modelTexture) {
                SDL_Log("Warning: Could not load model texture from %s", texPath.c_str());
            }
            
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);
            
            glBindVertexArray(VAO);
            
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ModelVertex), vertices.data(), GL_STATIC_DRAW);
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
            
            
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, pos));
            glEnableVertexAttribArray(0);
            
            
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, texCoord));
            glEnableVertexAttribArray(1);
            
            
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, normal));
            glEnableVertexAttribArray(2);
            
            glBindVertexArray(0);
            
            initialized = true;
            SDL_Log("GL_Model initialized with %zu vertices", vertices.size());
        }
        
        void draw(int screenW, int screenH, const glm::vec3& cameraPos, float cameraYaw, float cameraPitch) override {
            if (!initialized) return;
            
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            
            program.setSilent(true);
            program.useProgram();
            
            
            float time = SDL_GetTicks() * 0.001f;
            float rotX = time * 10.0f;
            float rotY = time * 30.0f;
            float rotZ = time * 5.0f;
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, modelPosition);
            model = glm::rotate(model, glm::radians(rotX), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rotZ), glm::vec3(0.0f, 0.0f, 1.0f));
            
            
            glm::vec3 front;
            front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
            front.y = sin(glm::radians(cameraPitch));
            front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
            front = glm::normalize(front);
            glm::mat4 view = glm::lookAt(cameraPos, cameraPos + front, glm::vec3(0.0f, 1.0f, 0.0f));
            
            
            glm::mat4 projection = glm::perspective(
                glm::radians(60.0f),
                (float)screenW / (float)screenH,
                0.1f, 10000.0f
            );
            
            program.setUniform("model", model);
            program.setUniform("view", view);
            program.setUniform("projection", projection);
            program.setUniform("time", static_cast<float>(SDL_GetTicks()) / 1000.0f);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, modelTexture);
            program.setUniform("modelTexture", 0);
            
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        
        void update(float deltaTime) override {
            rotationY += 30.0f * deltaTime;
            if (rotationY >= 360.0f) rotationY -= 360.0f;
        }
        
        void cleanup() override {
            if (!initialized) return;
            
            if (VAO) {
                glDeleteVertexArrays(1, &VAO);
                VAO = 0;
            }
            if (VBO) {
                glDeleteBuffers(1, &VBO);
                VBO = 0;
            }
            if (EBO) {
                glDeleteBuffers(1, &EBO);
                EBO = 0;
            }
            if (modelTexture) {
                glDeleteTextures(1, &modelTexture);
                modelTexture = 0;
            }
            
            vertices.clear();
            indices.clear();
            initialized = false;
        }
        
        bool isInitialized() const override { return initialized; }
        
        void setPosition(const glm::vec3& pos) override { modelPosition = pos; }
        glm::vec3 getPosition() const override { return modelPosition; }
        void setScale(float scale) override { modelScale = scale; }
        float getScale() const override { return modelScale; }
        void setRotation(float x, float y, float z) override { 
            rotationX = x; 
            rotationY = y; 
            rotationZ = z; 
        }
        void getRotation(float& x, float& y, float& z) const override { 
            x = rotationX; 
            y = rotationY; 
            z = rotationZ; 
        }
    };
#endif

#ifdef WITH_VK
    class VK_Window : public WindowBase, public mx::VKWindow {
    public:
        VK_Window() = default;
        
        void init(const std::string &title, const std::string &path, int w, int h, bool full) override {
            dataPath = path;
            mx::VKWindow::setPath(path);
            mx::VKWindow::initWindow(title, w, h, full);
            mx::VKWindow::initVulkan();
            if (handler) handler->load();
        }
        
        void loop() override {
            mx::VKWindow::loop();
        }
        
        void quit() override {
            mx::VKWindow::quit();
        }
        
        void cleanup() override {
            if (handler) handler->cleanup();
            mx::VKWindow::cleanup();
        }
        
        int getWidth() const override { return mx::VKWindow::w; }
        int getHeight() const override { return mx::VKWindow::h; }
        
        void setHandler(EventHandler *h) override { 
            handler = h;
            if (handler) handler->setWindow(this);
        }
        
        mx::mxUtil& getUtil() override { return mx::VKWindow::util; }
        const std::string& getPath() const override { return dataPath; }
        
        void event(SDL_Event &e) override {
            if (e.type == SDL_QUIT) {
                quit();
                return;
            }
            if (handler) handler->event(e);
        }
        
        void proc() override {
            static Uint64 lastTime = SDL_GetPerformanceCounter();
            Uint64 currentTime = SDL_GetPerformanceCounter();
            float deltaTime = (currentTime - lastTime) / (float)SDL_GetPerformanceFrequency();
            lastTime = currentTime;
            if (deltaTime > 0.1f) deltaTime = 0.1f;
            if (handler) handler->update(deltaTime);
        }
        
        
        void draw() override {
            currentImageIndex = 0;
            VkResult result = vkAcquireNextImageKHR(getDevice(), getSwapChain(), UINT64_MAX, 
                getImageAvailableSemaphore(), VK_NULL_HANDLE, &currentImageIndex);

            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                recreateSwapChain();
                return;
            }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                throw mx::Exception("Failed to acquire swap chain image!");
            }

            currentCommandBuffer = getCommandBuffer(currentImageIndex);
            vkResetCommandBuffer(currentCommandBuffer, 0);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            if (vkBeginCommandBuffer(currentCommandBuffer, &beginInfo) != VK_SUCCESS) {
                throw mx::Exception("Failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = getRenderPass();
            renderPassInfo.framebuffer = getSwapChainFramebuffer(currentImageIndex);
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = getSwapChainExtent();
            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clearValues[1].depthStencil = {1.0f, 0};
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();
            vkCmdBeginRenderPass(currentCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            
            if (handler) handler->draw();

            vkCmdEndRenderPass(currentCommandBuffer);
            if (vkEndCommandBuffer(currentCommandBuffer) != VK_SUCCESS) {
                throw mx::Exception("Failed to record command buffer!");
            }

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkSemaphore waitSemaphores[] = { getImageAvailableSemaphore() };
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &currentCommandBuffer;
            VkSemaphore signalSemaphores[] = { getRenderFinishedSemaphore() };
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            if (vkQueueSubmit(getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
                throw mx::Exception("Failed to submit draw command buffer!");
            }

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;
            VkSwapchainKHR swapChains[] = { getSwapChain() };
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &currentImageIndex;

            result = vkQueuePresentKHR(getPresentQueue(), &presentInfo);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                recreateSwapChain();
            }
            else if (result != VK_SUCCESS) {
                throw mx::Exception("Failed to present swap chain image!");
            }
            
            vkQueueWaitIdle(getPresentQueue());
        }
        
        
        VkCommandBuffer getCurrentCommandBuffer() const { return currentCommandBuffer; }
        uint32_t getCurrentImageIndex() const { return currentImageIndex; }
        
    private:
        EventHandler *handler = nullptr;
        std::string dataPath;
        VkCommandBuffer currentCommandBuffer = VK_NULL_HANDLE;
        uint32_t currentImageIndex = 0;
    };

    
    
    
    class VK_Model : public ModelBase {
    public:
        mx::VKWindow* vkWindow = nullptr;
        VkDevice device = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        
        std::vector<ModelVertex> vertices;
        std::vector<uint32_t> indices;
        
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
        
        VkImage modelTexture = VK_NULL_HANDLE;
        VkDeviceMemory modelTextureMemory = VK_NULL_HANDLE;
        VkImageView modelTextureView = VK_NULL_HANDLE;
        VkSampler modelSampler = VK_NULL_HANDLE;
        
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void*> uniformBuffersMapped;
        
        bool initialized = false;
        float rotationX = 0.0f;
        float rotationY = 0.0f;
        float rotationZ = 0.0f;
        float modelScale = 10.0f;
        glm::vec3 modelPosition = glm::vec3(0.0f, 0.0f, -30.0f);
        bool useEffectShader = false;
        std::string dataPathCache;
        
        ~VK_Model() override {
            cleanup();
        }
        
        void cleanup() override {
            if (!initialized || device == VK_NULL_HANDLE) return;
            
            SDL_Log("VK_Model::cleanup() - start");
            vkDeviceWaitIdle(device);
            
            if (pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(device, pipeline, nullptr);
                pipeline = VK_NULL_HANDLE;
            }
            if (pipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
                pipelineLayout = VK_NULL_HANDLE;
            }
            if (descriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(device, descriptorPool, nullptr);
                descriptorPool = VK_NULL_HANDLE;
            }
            descriptorSets.clear();
            if (descriptorSetLayout != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
                descriptorSetLayout = VK_NULL_HANDLE;
            }
            if (vertexBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, vertexBuffer, nullptr);
                vkFreeMemory(device, vertexBufferMemory, nullptr);
                vertexBuffer = VK_NULL_HANDLE;
            }
            if (indexBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, indexBuffer, nullptr);
                vkFreeMemory(device, indexBufferMemory, nullptr);
                indexBuffer = VK_NULL_HANDLE;
            }
            if (modelSampler != VK_NULL_HANDLE) {
                vkDestroySampler(device, modelSampler, nullptr);
                modelSampler = VK_NULL_HANDLE;
            }
            if (modelTextureView != VK_NULL_HANDLE) {
                vkDestroyImageView(device, modelTextureView, nullptr);
                modelTextureView = VK_NULL_HANDLE;
            }
            if (modelTexture != VK_NULL_HANDLE) {
                vkDestroyImage(device, modelTexture, nullptr);
                vkFreeMemory(device, modelTextureMemory, nullptr);
                modelTexture = VK_NULL_HANDLE;
            }
            for (size_t i = 0; i < uniformBuffers.size(); ++i) {
                if (uniformBuffers[i] != VK_NULL_HANDLE) {
                    vkDestroyBuffer(device, uniformBuffers[i], nullptr);
                }
                if (uniformBuffersMemory[i] != VK_NULL_HANDLE) {
                    vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
                }
            }
            uniformBuffers.clear();
            uniformBuffersMemory.clear();
            uniformBuffersMapped.clear();
            
            vertices.clear();
            indices.clear();
            
            SDL_Log("VK_Model::cleanup() - complete");
            initialized = false;
            device = VK_NULL_HANDLE;
        }
        
        void init(const std::string& dataPath, const std::string& modelFile, 
                 const std::string& textureFile, float scale = 1.0f, 
                 const glm::vec3& position = glm::vec3(0.0f, 0.0f, -5.0f)) override {
            throw mx::Exception("VK_Model::init() requires VK_Window pointer - use initWithWindow() instead");
        }
        
        void initWithWindow(mx::VKWindow* vkWin, const std::string& dataPath, 
                           const std::string& modelFile, const std::string& textureFile,
                           float scale = 10.0f, const glm::vec3& position = glm::vec3(0.0f, 0.0f, -30.0f)) {
            if (initialized) return;
            
            vkWindow = vkWin;
            device = vkWin->getDevice();
            physicalDevice = vkWin->getPhysicalDevice();
            commandPool = vkWin->getCommandPool();
            graphicsQueue = vkWin->getGraphicsQueue();
            dataPathCache = dataPath;
            
            modelScale = scale;
            modelPosition = position;
            std::string modelPath = dataPath + "/data/" + modelFile;
            loadMXMOD(modelPath, vertices, indices, modelScale);
            
            std::string texPath = dataPath + "/data/" + textureFile;
            loadTextureInternal(texPath);
            
            
            if (vert_shader.empty()) {
                vert_shader = dataPath + "/data/vert.spv";
            }
            if (frag_shader.empty()) {
                frag_shader = dataPath + "/data/frag.spv";
            }
            
            createDescriptorSetLayout();
            createPipeline(vkWin, dataPath);
            createVertexBuffer();
            createIndexBuffer();
            createUniformBuffers(vkWin);
            createDescriptorPool(vkWin);
            createDescriptorSets(vkWin);
            
            initialized = true;
            SDL_Log("VK_Model initialized with %zu vertices", vertices.size());
        }
        
        void loadTextureInternal(const std::string& path) {
            SDL_Surface* img = png::LoadPNG(path.c_str());
            if (!img || !img->pixels) {
                throw mx::Exception("Failed to load model texture: " + path);
            }
            
            int width = img->w;
            int height = img->h;
            VkDeviceSize imageSize = width * height * 4;
            
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = imageSize;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            
            VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer));
            
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);
            
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            
            VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory));
            vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);
            
            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, img->pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(device, stagingBufferMemory);
            
            SDL_FreeSurface(img);
            
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = static_cast<uint32_t>(width);
            imageInfo.extent.height = static_cast<uint32_t>(height);
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            
            VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &modelTexture));
            
            vkGetImageMemoryRequirements(device, modelTexture, &memRequirements);
            
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            
            VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &modelTextureMemory));
            vkBindImageMemory(device, modelTexture, modelTextureMemory, 0);
            
            transitionImageLayout(modelTexture, VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            copyBufferToImage(stagingBuffer, modelTexture, 
                static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            transitionImageLayout(modelTexture, VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
            
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = modelTexture;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;
            
            VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &modelTextureView));
            
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
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            
            VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &modelSampler));
        }
        
        void transitionImageLayout(VkImage image, VkFormat format,
            VkImageLayout oldLayout, VkImageLayout newLayout) {
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
            
            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
                newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                       newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            } else {
                throw mx::Exception("Unsupported layout transition");
            }
            
            vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage,
                0, 0, nullptr, 0, nullptr, 1, &barrier);
            
            vkEndCommandBuffer(commandBuffer);
            
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;
            
            vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphicsQueue);
            
            vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        }
        
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
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
            
            vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            
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
            std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
            
            bindings[0].binding = 0;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[0].descriptorCount = 1;
            bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[0].pImmutableSamplers = nullptr;
            
            bindings[1].binding = 1;
            bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            bindings[1].descriptorCount = 1;
            bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[1].pImmutableSamplers = nullptr;
            
            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            layoutInfo.pBindings = bindings.data();
            
            VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout));
        }
        
        void createDescriptorPool(mx::VKWindow* vkWin) {
            uint32_t imageCount = vkWin->getSwapChainImageCount();
            
            std::array<VkDescriptorPoolSize, 2> poolSizes{};
            poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            poolSizes[0].descriptorCount = imageCount;
            poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSizes[1].descriptorCount = imageCount;
            
            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.maxSets = imageCount;
            
            VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));
        }
        
        void createDescriptorSets(mx::VKWindow* vkWin) {
            uint32_t imageCount = vkWin->getSwapChainImageCount();
            std::vector<VkDescriptorSetLayout> layouts(imageCount, descriptorSetLayout);
            
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = descriptorPool;
            allocInfo.descriptorSetCount = imageCount;
            allocInfo.pSetLayouts = layouts.data();
            
            descriptorSets.resize(imageCount);
            VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()));
            
            for (size_t i = 0; i < imageCount; i++) {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = uniformBuffers[i];
                bufferInfo.offset = 0;
                bufferInfo.range = sizeof(mx::UniformBufferObject);
                
                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = modelTextureView;
                imageInfo.sampler = modelSampler;
                
                std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
                
                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstSet = descriptorSets[i];
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[0].descriptorCount = 1;
                descriptorWrites[0].pImageInfo = &imageInfo;
                
                descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[1].dstSet = descriptorSets[i];
                descriptorWrites[1].dstBinding = 1;
                descriptorWrites[1].dstArrayElement = 0;
                descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[1].descriptorCount = 1;
                descriptorWrites[1].pBufferInfo = &bufferInfo;
                
                vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                                       descriptorWrites.data(), 0, nullptr);
            }
        }
        
        void createVertexBuffer() {
            VkDeviceSize bufferSize = sizeof(ModelVertex) * vertices.size();
            
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferMemory);
            
            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
            vkUnmapMemory(device, stagingBufferMemory);
            
            createBuffer(bufferSize, 
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                vertexBuffer, vertexBufferMemory);
            
            copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
            
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
        }
        
        void createIndexBuffer() {
            VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();
            
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferMemory);
            
            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
            vkUnmapMemory(device, stagingBufferMemory);
            
            createBuffer(bufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                indexBuffer, indexBufferMemory);
            
            copyBuffer(stagingBuffer, indexBuffer, bufferSize);
            
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
        }

        void createUniformBuffers(mx::VKWindow* vkWin) {
            uint32_t imageCount = vkWin->getSwapChainImageCount();
            VkDeviceSize bufferSize = sizeof(mx::UniformBufferObject);
            uniformBuffers.resize(imageCount);
            uniformBuffersMemory.resize(imageCount);
            uniformBuffersMapped.resize(imageCount);

            for (size_t i = 0; i < imageCount; ++i) {
                createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    uniformBuffers[i], uniformBuffersMemory[i]);
                vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
            }
        }

        void updateModelUniformBuffer(uint32_t imageIndex, const mx::UniformBufferObject& ubo) {
            if (uniformBuffersMapped.size() > imageIndex && uniformBuffersMapped[imageIndex] != nullptr) {
                memcpy(uniformBuffersMapped[imageIndex], &ubo, sizeof(ubo));
            }
        }
        
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                          VkMemoryPropertyFlags properties,
                          VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            
            VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));
            
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
            
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
            
            VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory));
            vkBindBufferMemory(device, buffer, bufferMemory, 0);
        }
        
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
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
            
            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
            
            vkEndCommandBuffer(commandBuffer);
            
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;
            
            vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphicsQueue);
            
            vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        }
        
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
            
            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
                if ((typeFilter & (1 << i)) &&
                    (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                    return i;
                }
            }
            
            throw mx::Exception("Failed to find suitable memory type");
        }

        std::string vert_shader, frag_shader;

        void setShaders(const std::string &vert, const std::string &frag) {
            vert_shader = vert;
            frag_shader = frag;
        }
        
        void createPipeline(mx::VKWindow* vkWin, const std::string& dataPath) {
            auto vertShaderCode = mx::readFile(vert_shader);
            auto fragShaderCode = mx::readFile(frag_shader);
            SDL_Log("Loading Vulkan shaders: %s and %s", vert_shader.c_str(), frag_shader.c_str());
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = vertShaderCode.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data());
            
            VkShaderModule vertShaderModule;
            VK_CHECK_RESULT(vkCreateShaderModule(device, &createInfo, nullptr, &vertShaderModule));
            
            createInfo.codeSize = fragShaderCode.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data());
            
            VkShaderModule fragShaderModule;
            VK_CHECK_RESULT(vkCreateShaderModule(device, &createInfo, nullptr, &fragShaderModule));
            
            VkPipelineShaderStageCreateInfo shaderStages[2] = {};
            shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            shaderStages[0].module = vertShaderModule;
            shaderStages[0].pName = "main";
            
            shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            shaderStages[1].module = fragShaderModule;
            shaderStages[1].pName = "main";
            
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(ModelVertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
            
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(ModelVertex, pos);
            
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(ModelVertex, texCoord);
            
            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(ModelVertex, normal);
            
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
            viewport.width = (float)vkWin->getSwapChainExtent().width;
            viewport.height = (float)vkWin->getSwapChainExtent().height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            
            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = vkWin->getSwapChainExtent();
            
            std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
            };
            
            VkPipelineDynamicStateCreateInfo dynamicState{};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamicState.pDynamicStates = dynamicStates.data();
            
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
            
            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;
            
            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;
            
            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_TRUE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.stencilTestEnable = VK_FALSE;
            
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
            
            VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));
            
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
            pipelineInfo.renderPass = vkWin->getRenderPass();
            pipelineInfo.subpass = 0;
            
            VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
            
            vkDestroyShaderModule(device, fragShaderModule, nullptr);
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
        }
        
        void update(float deltaTime) override {
            rotationX += 10.0f * deltaTime;
            rotationY += 30.0f * deltaTime;
            rotationZ += 5.0f * deltaTime;
            if (rotationX >= 360.0f) rotationX -= 360.0f;
            if (rotationY >= 360.0f) rotationY -= 360.0f;
            if (rotationZ >= 360.0f) rotationZ -= 360.0f;
        }
        
        
        void draw(int screenW, int screenH, const glm::vec3& cameraPos, 
                 float cameraYaw, float cameraPitch) override {
            
            SDL_Log("Warning: VK_Model::draw() called - use drawWithCommandBuffer() instead");
        }
        
        
        void drawWithCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, 
                                   int screenW, int screenH,
                                   const glm::vec3& cameraPos, float cameraYaw, float cameraPitch) {
            if (!initialized || !vkWindow) {
                SDL_Log("VK_Model::draw() - not initialized or no window");
                return;
            }
        
            glm::vec3 front;
            front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
            front.y = sin(glm::radians(cameraPitch));
            front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
            front = glm::normalize(front);
            glm::mat4 view = glm::lookAt(cameraPos, cameraPos + front, glm::vec3(0.0f, 1.0f, 0.0f));
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, modelPosition);
            model = glm::rotate(model, glm::radians(rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rotationZ), glm::vec3(0.0f, 0.0f, 1.0f));
            
            glm::mat4 proj = glm::perspective(glm::radians(60.0f),
                (float)screenW / (float)screenH, 0.1f, 10000.0f);
            proj[1][1] *= -1; 
            
            mx::UniformBufferObject ubo{};
            ubo.model = model;
            ubo.view = view;
            ubo.proj = proj;
            ubo.color = glm::vec4(1.0f);
            ubo.params = glm::vec4(SDL_GetTicks() / 1000.0f, 0.0f, 0.0f, 0.0f);
            updateModelUniformBuffer(imageIndex, ubo);
            
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(screenW);
            viewport.height = static_cast<float>(screenH);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            
            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = {static_cast<uint32_t>(screenW), static_cast<uint32_t>(screenH)};
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            
            VkBuffer vertexBuffers[] = {vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout, 0, 1, &descriptorSets[imageIndex], 0, nullptr);
            
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        }
        
        void switchShader(const std::string &v, const std::string &f) {
            if (!initialized || device == VK_NULL_HANDLE) return;
            
            vkDeviceWaitIdle(device);
            
            if (pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(device, pipeline, nullptr);
                pipeline = VK_NULL_HANDLE;
            }
            if (pipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
                pipelineLayout = VK_NULL_HANDLE;
            }
            setShaders(v, f);
            createPipeline(vkWindow, dataPathCache);
        }
            
        bool isInitialized() const override { return initialized; }
        
        void setPosition(const glm::vec3& pos) override { modelPosition = pos; }
        glm::vec3 getPosition() const override { return modelPosition; }
        void setScale(float scale) override { modelScale = scale; }
        float getScale() const override { return modelScale; }
        void setRotation(float x, float y, float z) override { 
            rotationX = x; 
            rotationY = y; 
            rotationZ = z; 
        }
        void getRotation(float& x, float& y, float& z) const override { 
            x = rotationX; 
            y = rotationY; 
            z = rotationZ; 
        }
    };
#endif

    inline std::unique_ptr<WindowBase> createWindow() {
#ifdef WITH_VK
        return std::make_unique<VK_Window>();
#elif defined(WITH_GL)
        return std::make_unique<GL_Window>();
#else
        static_assert(false, "Either WITH_GL or WITH_VK must be defined");
        return nullptr;
#endif
    }

    
    
    
    inline std::unique_ptr<ModelBase> createModel() {
#ifdef WITH_VK
        return std::make_unique<VK_Model>();
#elif defined(WITH_GL)
        return std::make_unique<GL_Model>();
#else
        static_assert(false, "Either WITH_GL or WITH_VK must be defined");
        return nullptr;
#endif
    }

#ifdef WITH_VK
    using MX_Window = VK_Window;
    using MX_Model = VK_Model;
#elif defined(WITH_GL)
    using MX_Window = GL_Window;
    using MX_Model = GL_Model;
#endif

}

#endif