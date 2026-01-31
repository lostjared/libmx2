#ifndef MXCROSS_H_
#define MXCROSS_H_
#ifndef __EMSCRIPTEN__
#include"config.h"
#endif
#include"mx.hpp"
#include"argz.hpp"
#ifdef WITH_GL
#include"gl.hpp"
#endif
#ifdef WITH_VK
#include"vk.hpp"
#endif

namespace cross {

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
            if (handler) handler->event(e);
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

#ifdef WITH_VK
    using MX_Window = VK_Window;
#elif defined(WITH_GL)
    using MX_Window = GL_Window;
#endif

}

#endif