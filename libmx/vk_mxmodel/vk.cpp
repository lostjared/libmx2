#include "vk.hpp"
#include "loadpng.hpp"

namespace mx {

    ModelViewer::ModelViewer(const std::string &title, int width, int height, bool full) {
        initWindow(title, width, height, full);
    }

    void ModelViewer::initWindow(const std::string &title, int width, int height, bool full) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
            throw mx::Exception("SDL_Init: " + std::string(SDL_GetError()));
        Uint32 flags = SDL_WINDOW_VULKAN;
        if (full) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
        if (!window) throw mx::Exception("SDL_CreateWindow: " + std::string(SDL_GetError()));
        w = width;
        h = height;
    }

    void ModelViewer::quit() { active = false; }

    void ModelViewer::setPath(const std::string &path) { util.path = path; }
    void ModelViewer::setModelFile(const std::string &filename) { modelFilename = filename; }
    void ModelViewer::setTextureFile(const std::string &filename) { textureFilename = filename; }

    void ModelViewer::initVulkan() {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createDepthResources();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipelines();
        createFramebuffers();
        createCommandPool();
        loadModelData();
        loadTextureData();
        createTextureImageView();
        createTextureSampler();
        createDescriptorPool();
        createUniformBuffers();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    
    void ModelViewer::loadModelData() {
        std::string path = modelFilename.empty() ? util.getFilePath("lattice.mxmod") : modelFilename;
        model.load(path, 1.0f);
        model.upload(device, physicalDevice, commandPool, graphicsQueue);
        std::cout << ">> Loaded model: " << path << " (" << model.indexCount() << " indices)\n";
    }

    void ModelViewer::loadTextureData() {
        std::string texArg = textureFilename.empty() ? util.getFilePath("bg.png") : textureFilename;

        
        std::vector<std::string> imagePaths;
        bool isTexFile = (texArg.size() >= 4 && texArg.substr(texArg.size() - 4) == ".tex");

        if (isTexFile) {
            
            std::ifstream tf(texArg);
            if (!tf.is_open()) throw mx::Exception("Failed to open .tex file: " + texArg);
            
            std::string prefix;
            auto slash = texArg.find_last_of("/\\");
            if (slash != std::string::npos)
                prefix = texArg.substr(0, slash);
            else
                prefix = util.path;

            std::string line;
            while (std::getline(tf, line)) {
                
                size_t b = line.find_first_not_of(" \t\r\n");
                if (b == std::string::npos) continue;
                size_t e = line.find_last_not_of(" \t\r\n");
                line = line.substr(b, e - b + 1);
                if (line.empty() || line[0] == '#') continue;
                imagePaths.push_back(prefix + "/" + line);
            }
            tf.close();
            if (imagePaths.empty())
                throw mx::Exception("No texture paths found in .tex file: " + texArg);
            std::cout << ">> Loaded .tex file: " << texArg << " (" << imagePaths.size() << " textures)\n";
        } else {
            imagePaths.push_back(texArg);
        }

        
        VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
        for (size_t i = 0; i < imagePaths.size(); ++i) {
            SDL_Surface *img = png::LoadPNG(imagePaths[i].c_str());
            if (!img) {
                std::cerr << ">> Warning: Failed to load texture: " << imagePaths[i] << ", using 1x1 white\n";
                
                img = SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 32, SDL_PIXELFORMAT_RGBA32);
                if (img) { uint32_t *px = (uint32_t*)img->pixels; *px = 0xFFFFFFFF; }
                else throw mx::Exception("Failed to create placeholder surface");
            }

            VkTexture tex{};
            tex.w = img->w;
            tex.h = img->h;
            VkDeviceSize imageSize = tex.w * tex.h * 4;

            createImage(tex.w, tex.h, fmt, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex.image, tex.memory);

            transitionImageLayout(tex.image, fmt, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkBuffer staging; VkDeviceMemory stagingMem;
            createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging, stagingMem);
            void *data;
            vkMapMemory(device, stagingMem, 0, imageSize, 0, &data);
            memcpy(data, img->pixels, (size_t)imageSize);
            vkUnmapMemory(device, stagingMem);
            copyBufferToImage(staging, tex.image, tex.w, tex.h);
            transitionImageLayout(tex.image, fmt, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            vkDestroyBuffer(device, staging, nullptr);
            vkFreeMemory(device, stagingMem, nullptr);
            SDL_FreeSurface(img);

            tex.view = createImageView(tex.image, fmt, VK_IMAGE_ASPECT_COLOR_BIT);
            textures.push_back(tex);
            std::cout << ">> Texture[" << i << "]: " << imagePaths[i] << " (" << tex.w << "x" << tex.h << ")\n";
        }

        
        uint32_t tidx = model.textureIndex();
        if (tidx < textures.size()) {
            textureImage = textures[tidx].image;
            textureImageMemory = textures[tidx].memory;
            textureImageView = textures[tidx].view;
            texWidth = textures[tidx].w;
            texHeight = textures[tidx].h;
            std::cout << ">> Using texture index " << tidx << " for model\n";
        } else {
            textureImage = textures[0].image;
            textureImageMemory = textures[0].memory;
            textureImageView = textures[0].view;
            texWidth = textures[0].w;
            texHeight = textures[0].h;
            if (tidx != 0)
                std::cerr << ">> Warning: model requests texture index " << tidx << " but only " << textures.size() << " loaded, using 0\n";
        }
    }

    
    void ModelViewer::event(SDL_Event &e) {
        if (e.type == SDL_QUIT) { quit(); return; }
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE: quit(); break;
                case SDLK_w:
                    wireframe = !wireframe;
                    std::cout << ">> Wireframe: " << (wireframe ? "ON" : "OFF") << "\n";
                    break;
                case SDLK_r:
                    autoRotate = !autoRotate;
                    std::cout << ">> Auto-rotate: " << (autoRotate ? "ON" : "OFF") << "\n";
                    break;
                case SDLK_UP:    rotationX -= 5.0f; break;
                case SDLK_DOWN:  rotationX += 5.0f; break;
                case SDLK_LEFT:  rotationY -= 5.0f; break;
                case SDLK_RIGHT: rotationY += 5.0f; break;
                case SDLK_EQUALS: case SDLK_PLUS:
                    cameraDistance -= 0.5f;
                    if (cameraDistance < 0.1f) cameraDistance = 0.1f;
                    break;
                case SDLK_MINUS:
                    cameraDistance += 0.5f;
                    if (cameraDistance > 100.0f) cameraDistance = 100.0f;
                    break;
                case SDLK_HOME:
                    rotationX = 0; rotationY = 0; cameraDistance = 5.0f;
                    break;
            }
        }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            mouseDown = true;
            lastMouseX = e.button.x;
            lastMouseY = e.button.y;
        }
        if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
            mouseDown = false;
        }
        if (e.type == SDL_MOUSEMOTION && mouseDown) {
            int dx = e.motion.x - lastMouseX;
            int dy = e.motion.y - lastMouseY;
            rotationY += dx * 0.5f;
            rotationX += dy * 0.5f;
            if (rotationX > 89.0f) rotationX = 89.0f;
            if (rotationX < -89.0f) rotationX = -89.0f;
            lastMouseX = e.motion.x;
            lastMouseY = e.motion.y;
        }
        if (e.type == SDL_MOUSEWHEEL) {
            cameraDistance -= e.wheel.y * 0.5f;
            if (cameraDistance < 0.1f) cameraDistance = 0.1f;
            if (cameraDistance > 100.0f) cameraDistance = 100.0f;
        }
    }

    void ModelViewer::loop() {
        SDL_Event e;
        while (active) {
            while (SDL_PollEvent(&e)) event(e);
            draw();
        }
    }

    
    void ModelViewer::draw() {
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) { recreateSwapChain(); return; }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw mx::Exception("Failed to acquire swap chain image!");

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

        vkCmdBeginRenderPass(commandBuffers[imageIndex], &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkPipeline activePipeline = wireframe ? wirePipeline : fillPipeline;
        vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, activePipeline);

        
        UniformBufferObject ubo{};
        float time = SDL_GetTicks() / 1000.0f;
        ubo.params = glm::vec4(time, (float)swapChainExtent.width, (float)swapChainExtent.height, 0.0f);
        ubo.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

        
        float autoAngle = autoRotate ? time * glm::radians(45.0f) : 0.0f;
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::rotate(modelMat, glm::radians(rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMat = glm::rotate(modelMat, glm::radians(rotationY) + autoAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.model = modelMat;

        ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, cameraDistance), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        float aspect = (float)swapChainExtent.width / (float)swapChainExtent.height;
        ubo.proj = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.0f);
        ubo.proj[1][1] *= -1;

        if (uniformBuffersMapped.size() > imageIndex && uniformBuffersMapped[imageIndex])
            memcpy(uniformBuffersMapped[imageIndex], &ubo, sizeof(ubo));

        if (!descriptorSets.empty())
            vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[imageIndex], 0, nullptr);

        model.draw(commandBuffers[imageIndex]);
        vkCmdEndRenderPass(commandBuffers[imageIndex]);

        if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS)
            throw mx::Exception("Failed to record command buffer!");

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSems[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSems;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        VkSemaphore signalSems[] = {renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSems;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
            throw mx::Exception("Failed to submit draw command buffer!");

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSems;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) recreateSwapChain();
        else if (result != VK_SUCCESS) throw mx::Exception("Failed to present swap chain image!");
        vkQueueWaitIdle(presentQueue);
    }

    
    void ModelViewer::createInstance() {
#ifndef WITH_MOLTEN
        if (volkInitialize() != VK_SUCCESS) throw mx::Exception("Failed to initialize Volk!");
#endif
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "MXModel Viewer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        unsigned int sdlExtCount = 0;
        SDL_Vulkan_GetInstanceExtensions(window, &sdlExtCount, nullptr);
        std::vector<const char*> extensions(sdlExtCount);
        SDL_Vulkan_GetInstanceExtensions(window, &sdlExtCount, extensions.data());

        const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#ifdef WITH_MOLTEN
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

        
        bool layersSupported = true;
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        for (const char* name : validationLayers) {
            bool found = false;
            for (const auto& l : availableLayers) if (strcmp(name, l.layerName) == 0) { found = true; break; }
            if (!found) { layersSupported = false; break; }
        }

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = (uint32_t)extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();
#ifdef WITH_MOLTEN
        createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
        if (layersSupported) {
            createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &instance));
#ifndef WITH_MOLTEN
        volkLoadInstance(instance);
#endif
    }

    void ModelViewer::createSurface() {
        if (!SDL_Vulkan_CreateSurface(window, instance, &surface))
            throw mx::Exception("Failed to create Vulkan surface!");
    }

    bool ModelViewer::isDeviceSuitable(VkPhysicalDevice dev) {
        QueueFamilyIndices indices = findQueueFamilies(dev);
        const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        uint32_t extCount;
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> avail(extCount);
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, avail.data());
        std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());
        for (const auto& e : avail) required.erase(e.extensionName);
        bool swapOk = false;
        if (required.empty()) {
            auto sc = querySwapChainSupport(dev);
            swapOk = !sc.formats.empty() && !sc.presentModes.empty();
        }
        return indices.isComplete() && required.empty() && swapOk;
    }

    void ModelViewer::pickPhysicalDevice() {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(instance, &count, nullptr);
        if (!count) throw mx::Exception("No Vulkan GPUs found!");
        std::vector<VkPhysicalDevice> devs(count);
        vkEnumeratePhysicalDevices(instance, &count, devs.data());
        for (auto &d : devs) if (isDeviceSuitable(d)) { physicalDevice = d; break; }
        if (physicalDevice == VK_NULL_HANDLE) throw mx::Exception("No suitable GPU!");
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physicalDevice, &props);
        std::cout << ">> GPU: " << props.deviceName << "\n";
    }

    void ModelViewer::createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        std::set<uint32_t> uniqueQueues = {indices.graphicsFamily.value(), indices.presentFamily.value()};
        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        float prio = 1.0f;
        for (uint32_t qf : uniqueQueues) {
            VkDeviceQueueCreateInfo qi{};
            qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qi.queueFamilyIndex = qf;
            qi.queueCount = 1;
            qi.pQueuePriorities = &prio;
            queueInfos.push_back(qi);
        }

        VkPhysicalDeviceFeatures features{};
        features.samplerAnisotropy = VK_TRUE;
        features.fillModeNonSolid = VK_TRUE;  

        std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#ifdef WITH_MOLTEN
        uint32_t extCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> avail(extCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, avail.data());
        for (auto &e : avail) if (strcmp(e.extensionName, "VK_KHR_portability_subset") == 0) { deviceExtensions.push_back("VK_KHR_portability_subset"); break; }
#endif

        VkDeviceCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        ci.queueCreateInfoCount = (uint32_t)queueInfos.size();
        ci.pQueueCreateInfos = queueInfos.data();
        ci.pEnabledFeatures = &features;
        ci.enabledExtensionCount = (uint32_t)deviceExtensions.size();
        ci.ppEnabledExtensionNames = deviceExtensions.data();

        if (vkCreateDevice(physicalDevice, &ci, nullptr, &device) != VK_SUCCESS)
            throw mx::Exception("Failed to create logical device!");
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
#ifndef WITH_MOLTEN
        volkLoadDevice(device);
#endif
    }

    
    QueueFamilyIndices ModelViewer::findQueueFamilies(VkPhysicalDevice dev) {
        QueueFamilyIndices idx;
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, families.data());
        int i = 0;
        for (auto &qf : families) {
            if (qf.queueFlags & VK_QUEUE_GRAPHICS_BIT) idx.graphicsFamily = i;
            VkBool32 present = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &present);
            if (present) idx.presentFamily = i;
            if (idx.isComplete()) break;
            i++;
        }
        return idx;
    }

    VkSurfaceFormatKHR ModelViewer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
        for (auto &f : formats)
            if (f.format == VK_FORMAT_R8G8B8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return f;
        return formats[0];
    }

    VkPresentModeKHR ModelViewer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes) {
        for (auto &m : modes) if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D ModelViewer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps) {
        if (caps.currentExtent.width != UINT32_MAX) return caps.currentExtent;
        VkExtent2D ext = {(uint32_t)w, (uint32_t)h};
        ext.width = std::max(caps.minImageExtent.width, std::min(caps.maxImageExtent.width, ext.width));
        ext.height = std::max(caps.minImageExtent.height, std::min(caps.maxImageExtent.height, ext.height));
        return ext;
    }

    SwapChainSupportDetails ModelViewer::querySwapChainSupport(VkPhysicalDevice dev) {
        SwapChainSupportDetails d;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &d.capabilities);
        uint32_t fc = 0; vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &fc, nullptr);
        if (fc) { d.formats.resize(fc); vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &fc, d.formats.data()); }
        uint32_t pc = 0; vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &pc, nullptr);
        if (pc) { d.presentModes.resize(pc); vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &pc, d.presentModes.data()); }
        return d;
    }

    void ModelViewer::createSwapChain() {
        auto sc = querySwapChainSupport(physicalDevice);
        auto fmt = chooseSwapSurfaceFormat(sc.formats);
        auto mode = chooseSwapPresentMode(sc.presentModes);
        auto extent = chooseSwapExtent(sc.capabilities);
        uint32_t imgCount = sc.capabilities.minImageCount + 1;
        if (sc.capabilities.maxImageCount > 0 && imgCount > sc.capabilities.maxImageCount)
            imgCount = sc.capabilities.maxImageCount;

        VkSwapchainCreateInfoKHR ci{};
        ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.surface = surface;
        ci.minImageCount = imgCount;
        ci.imageFormat = fmt.format;
        ci.imageColorSpace = fmt.colorSpace;
        ci.imageExtent = extent;
        ci.imageArrayLayers = 1;
        ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        auto idx = findQueueFamilies(physicalDevice);
        uint32_t qfi[] = {idx.graphicsFamily.value(), idx.presentFamily.value()};
        if (idx.graphicsFamily != idx.presentFamily) {
            ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            ci.queueFamilyIndexCount = 2;
            ci.pQueueFamilyIndices = qfi;
        } else {
            ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        ci.preTransform = sc.capabilities.currentTransform;
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        ci.presentMode = mode;
        ci.clipped = VK_TRUE;
        ci.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &ci, nullptr, &swapChain) != VK_SUCCESS)
            throw mx::Exception("Failed to create swap chain!");
        vkGetSwapchainImagesKHR(device, swapChain, &imgCount, nullptr);
        swapChainImages.resize(imgCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imgCount, swapChainImages.data());
        swapChainImageFormat = fmt.format;
        swapChainExtent = extent;
    }

    void ModelViewer::createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.image = swapChainImages[i];
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.format = swapChainImageFormat;
            ci.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
            ci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            if (vkCreateImageView(device, &ci, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
                throw mx::Exception("Failed to create image views!");
        }
    }

    
    void ModelViewer::createRenderPass() {
        VkAttachmentDescription colorAtt{};
        colorAtt.format = swapChainImageFormat;
        colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depthAtt{};
        depthAtt.format = depthFormat;
        depthAtt.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAtt.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAtt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;

        VkSubpassDependency dep{};
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> atts = {colorAtt, depthAtt};
        VkRenderPassCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        ci.attachmentCount = (uint32_t)atts.size();
        ci.pAttachments = atts.data();
        ci.subpassCount = 1;
        ci.pSubpasses = &subpass;
        ci.dependencyCount = 1;
        ci.pDependencies = &dep;

        if (vkCreateRenderPass(device, &ci, nullptr, &renderPass) != VK_SUCCESS)
            throw mx::Exception("Failed to create render pass!");
    }

    
    VkFormat ModelViewer::findSupportedFormat(const std::vector<VkFormat>& cands, VkImageTiling tiling, VkFormatFeatureFlags feats) {
        for (auto f : cands) {
            VkFormatProperties p;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, f, &p);
            if (tiling == VK_IMAGE_TILING_LINEAR && (p.linearTilingFeatures & feats) == feats) return f;
            if (tiling == VK_IMAGE_TILING_OPTIMAL && (p.optimalTilingFeatures & feats) == feats) return f;
        }
        throw mx::Exception("Failed to find supported format!");
    }

    VkFormat ModelViewer::findDepthFormat() {
        return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    void ModelViewer::createDepthResources() {
        depthFormat = findDepthFormat();
        createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    
    void ModelViewer::createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 0;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.descriptorCount = 1;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding = 1;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = 1;
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {samplerBinding, uboBinding};
        VkDescriptorSetLayoutCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ci.bindingCount = (uint32_t)bindings.size();
        ci.pBindings = bindings.data();
        if (vkCreateDescriptorSetLayout(device, &ci, nullptr, &descriptorSetLayout) != VK_SUCCESS)
            throw mx::Exception("Failed to create descriptor set layout!");
    }

    VkShaderModule ModelViewer::createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = code.size();
        ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule mod;
        if (vkCreateShaderModule(device, &ci, nullptr, &mod) != VK_SUCCESS)
            throw mx::Exception("Failed to create shader module!");
        return mod;
    }

    VkPipeline ModelViewer::buildPipeline(VkPolygonMode polyMode) {
        auto vertCode = mx::readFile(util.getFilePath("vert.spv"));
        auto fragCode = mx::readFile(util.getFilePath("frag.spv"));
        VkShaderModule vertMod = createShaderModule(vertCode);
        VkShaderModule fragMod = createShaderModule(fragCode);

        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertMod;
        vertStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragStage{};
        fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = fragMod;
        fragStage.pName = "main";

        VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(Vertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 3> attrs{};
        attrs[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)};
        attrs[1] = {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)};
        attrs[2] = {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)};

        VkPipelineVertexInputStateCreateInfo vertInput{};
        vertInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertInput.vertexBindingDescriptionCount = 1;
        vertInput.pVertexBindingDescriptions = &binding;
        vertInput.vertexAttributeDescriptionCount = (uint32_t)attrs.size();
        vertInput.pVertexAttributeDescriptions = attrs.data();

        VkPipelineInputAssemblyStateCreateInfo inputAsm{};
        inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAsm.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport vp{0, 0, (float)swapChainExtent.width, (float)swapChainExtent.height, 0, 1};
        VkRect2D scissor{{0, 0}, swapChainExtent};

        VkPipelineViewportStateCreateInfo vpState{};
        vpState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vpState.viewportCount = 1;
        vpState.pViewports = &vp;
        vpState.scissorCount = 1;
        vpState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rast{};
        rast.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rast.polygonMode = polyMode;
        rast.lineWidth = 1.0f;
        rast.cullMode = VK_CULL_MODE_NONE;
        rast.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo ms{};
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo ds{};
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.depthTestEnable = VK_TRUE;
        ds.depthWriteEnable = VK_TRUE;
        ds.depthCompareOp = VK_COMPARE_OP_LESS;

        VkPipelineColorBlendAttachmentState blend{};
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo cb{};
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.attachmentCount = 1;
        cb.pAttachments = &blend;

        VkGraphicsPipelineCreateInfo pi{};
        pi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pi.stageCount = 2;
        pi.pStages = stages;
        pi.pVertexInputState = &vertInput;
        pi.pInputAssemblyState = &inputAsm;
        pi.pViewportState = &vpState;
        pi.pRasterizationState = &rast;
        pi.pMultisampleState = &ms;
        pi.pDepthStencilState = &ds;
        pi.pColorBlendState = &cb;
        pi.layout = pipelineLayout;
        pi.renderPass = renderPass;
        pi.subpass = 0;

        VkPipeline pipeline;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pi, nullptr, &pipeline) != VK_SUCCESS)
            throw mx::Exception("Failed to create graphics pipeline!");

        vkDestroyShaderModule(device, fragMod, nullptr);
        vkDestroyShaderModule(device, vertMod, nullptr);
        return pipeline;
    }

    void ModelViewer::createGraphicsPipelines() {
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &descriptorSetLayout;
        if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
            throw mx::Exception("Failed to create pipeline layout!");

        fillPipeline = buildPipeline(VK_POLYGON_MODE_FILL);
        wirePipeline = buildPipeline(VK_POLYGON_MODE_LINE);
        std::cout << ">> Created fill + wireframe pipelines\n";
    }

    
    void ModelViewer::createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::array<VkImageView, 2> atts = {swapChainImageViews[i], depthImageView};
            VkFramebufferCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            ci.renderPass = renderPass;
            ci.attachmentCount = (uint32_t)atts.size();
            ci.pAttachments = atts.data();
            ci.width = swapChainExtent.width;
            ci.height = swapChainExtent.height;
            ci.layers = 1;
            if (vkCreateFramebuffer(device, &ci, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
                throw mx::Exception("Failed to create framebuffer!");
        }
    }

    void ModelViewer::createCommandPool() {
        auto idx = findQueueFamilies(physicalDevice);
        VkCommandPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        ci.queueFamilyIndex = idx.graphicsFamily.value();
        ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        if (vkCreateCommandPool(device, &ci, nullptr, &commandPool) != VK_SUCCESS)
            throw mx::Exception("Failed to create command pool!");
    }

    void ModelViewer::createCommandBuffers() {
        commandBuffers.resize(swapChainFramebuffers.size());
        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = commandPool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = (uint32_t)commandBuffers.size();
        if (vkAllocateCommandBuffers(device, &ai, commandBuffers.data()) != VK_SUCCESS)
            throw mx::Exception("Failed to allocate command buffers!");
    }

    void ModelViewer::createSyncObjects() {
        VkSemaphoreCreateInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vkCreateSemaphore(device, &si, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &si, nullptr, &renderFinishedSemaphore) != VK_SUCCESS)
            throw mx::Exception("Failed to create semaphores!");
    }

    
    void ModelViewer::createUniformBuffers() {
        VkDeviceSize size = sizeof(UniformBufferObject);
        size_t count = swapChainImages.size();
        uniformBuffers.resize(count);
        uniformBuffersMemory.resize(count);
        uniformBuffersMapped.resize(count);
        for (size_t i = 0; i < count; i++) {
            createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                uniformBuffers[i], uniformBuffersMemory[i]);
            vkMapMemory(device, uniformBuffersMemory[i], 0, size, 0, &uniformBuffersMapped[i]);
        }
    }

    
    void ModelViewer::createTextureImageView() {
        
        if (textureImageView != VK_NULL_HANDLE) return;
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void ModelViewer::createTextureSampler() {
        VkSamplerCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        ci.magFilter = VK_FILTER_LINEAR;
        ci.minFilter = VK_FILTER_LINEAR;
        ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        ci.anisotropyEnable = VK_TRUE;
        ci.maxAnisotropy = 16.0f;
        ci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        if (vkCreateSampler(device, &ci, nullptr, &textureSampler) != VK_SUCCESS)
            throw mx::Exception("Failed to create texture sampler!");
    }

    
    void ModelViewer::createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 2> sizes{};
        sizes[0] = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, (uint32_t)swapChainImages.size()};
        sizes[1] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (uint32_t)swapChainImages.size()};

        VkDescriptorPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ci.poolSizeCount = (uint32_t)sizes.size();
        ci.pPoolSizes = sizes.data();
        ci.maxSets = (uint32_t)swapChainImages.size();
        if (vkCreateDescriptorPool(device, &ci, nullptr, &descriptorPool) != VK_SUCCESS)
            throw mx::Exception("Failed to create descriptor pool!");
    }

    void ModelViewer::createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
        VkDescriptorSetAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool = descriptorPool;
        ai.descriptorSetCount = (uint32_t)swapChainImages.size();
        ai.pSetLayouts = layouts.data();
        descriptorSets.resize(swapChainImages.size());
        if (vkAllocateDescriptorSets(device, &ai, descriptorSets.data()) != VK_SUCCESS)
            throw mx::Exception("Failed to allocate descriptor sets!");

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkDescriptorImageInfo imgInfo{};
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imgInfo.imageView = textureImageView;
            imgInfo.sampler = textureSampler;

            VkDescriptorBufferInfo bufInfo{};
            bufInfo.buffer = uniformBuffers[i];
            bufInfo.offset = 0;
            bufInfo.range = sizeof(UniformBufferObject);

            std::array<VkWriteDescriptorSet, 2> writes{};
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = descriptorSets[i];
            writes[0].dstBinding = 0;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[0].descriptorCount = 1;
            writes[0].pImageInfo = &imgInfo;

            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = descriptorSets[i];
            writes[1].dstBinding = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[1].descriptorCount = 1;
            writes[1].pBufferInfo = &bufInfo;

            vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
        }
    }

    
    void ModelViewer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkBuffer& buf, VkDeviceMemory& mem) {
        VkBufferCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        ci.size = size;
        ci.usage = usage;
        ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device, &ci, nullptr, &buf) != VK_SUCCESS)
            throw mx::Exception("Failed to create buffer!");
        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(device, buf, &req);
        VkMemoryAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = req.size;
        ai.memoryTypeIndex = findMemoryType(req.memoryTypeBits, props);
        if (vkAllocateMemory(device, &ai, nullptr, &mem) != VK_SUCCESS)
            throw mx::Exception("Failed to allocate buffer memory!");
        vkBindBufferMemory(device, buf, mem, 0);
    }

    uint32_t ModelViewer::findMemoryType(uint32_t filter, VkMemoryPropertyFlags props) {
        VkPhysicalDeviceMemoryProperties mp;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &mp);
        for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
            if ((filter & (1 << i)) && (mp.memoryTypes[i].propertyFlags & props) == props) return i;
        throw mx::Exception("Failed to find suitable memory type!");
    }

    void ModelViewer::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
        VkCommandBuffer cmd = beginSingleTimeCommands();
        VkBufferCopy region{};
        region.size = size;
        vkCmdCopyBuffer(cmd, src, dst, 1, &region);
        endSingleTimeCommands(cmd);
    }

    void ModelViewer::createImage(uint32_t w, uint32_t h, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags props, VkImage& image, VkDeviceMemory& mem) {
        VkImageCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ci.imageType = VK_IMAGE_TYPE_2D;
        ci.extent = {w, h, 1};
        ci.mipLevels = 1;
        ci.arrayLayers = 1;
        ci.format = format;
        ci.tiling = tiling;
        ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ci.usage = usage;
        ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ci.samples = VK_SAMPLE_COUNT_1_BIT;
        if (vkCreateImage(device, &ci, nullptr, &image) != VK_SUCCESS) throw mx::Exception("Failed to create image!");
        VkMemoryRequirements req;
        vkGetImageMemoryRequirements(device, image, &req);
        VkMemoryAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = req.size;
        ai.memoryTypeIndex = findMemoryType(req.memoryTypeBits, props);
        if (vkAllocateMemory(device, &ai, nullptr, &mem) != VK_SUCCESS) throw mx::Exception("Failed to allocate image memory!");
        vkBindImageMemory(device, image, mem, 0);
    }

    void ModelViewer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldL, VkImageLayout newL) {
        VkCommandBuffer cmd = beginSingleTimeCommands();
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldL;
        barrier.newLayout = newL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VkPipelineStageFlags srcStage, dstStage;
        if (oldL == VK_IMAGE_LAYOUT_UNDEFINED && newL == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldL == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newL == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw mx::Exception("Unsupported layout transition!");
        }
        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        endSingleTimeCommands(cmd);
    }

    void ModelViewer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t w, uint32_t h) {
        VkCommandBuffer cmd = beginSingleTimeCommands();
        VkBufferImageCopy region{};
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageExtent = {w, h, 1};
        vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        endSingleTimeCommands(cmd);
    }

    VkCommandBuffer ModelViewer::beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandPool = commandPool;
        ai.commandBufferCount = 1;
        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &ai, &cmd);
        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &bi);
        return cmd;
    }

    void ModelViewer::endSingleTimeCommands(VkCommandBuffer cmd) {
        vkEndCommandBuffer(cmd);
        VkSubmitInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cmd;
        vkQueueSubmit(graphicsQueue, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
    }

    VkImageView ModelViewer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect) {
        VkImageViewCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ci.image = image;
        ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ci.format = format;
        ci.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        ci.subresourceRange = {aspect, 0, 1, 0, 1};
        VkImageView view;
        if (vkCreateImageView(device, &ci, nullptr, &view) != VK_SUCCESS)
            throw mx::Exception("Failed to create image view!");
        return view;
    }

    
    void ModelViewer::cleanupSwapChain() {
        for (auto fb : swapChainFramebuffers) if (fb) vkDestroyFramebuffer(device, fb, nullptr);
        swapChainFramebuffers.clear();
        if (!commandBuffers.empty()) {
            vkFreeCommandBuffers(device, commandPool, (uint32_t)commandBuffers.size(), commandBuffers.data());
            commandBuffers.clear();
        }
        if (fillPipeline) { vkDestroyPipeline(device, fillPipeline, nullptr); fillPipeline = VK_NULL_HANDLE; }
        if (wirePipeline) { vkDestroyPipeline(device, wirePipeline, nullptr); wirePipeline = VK_NULL_HANDLE; }
        if (pipelineLayout) { vkDestroyPipelineLayout(device, pipelineLayout, nullptr); pipelineLayout = VK_NULL_HANDLE; }
        if (renderPass) { vkDestroyRenderPass(device, renderPass, nullptr); renderPass = VK_NULL_HANDLE; }
        for (auto iv : swapChainImageViews) if (iv) vkDestroyImageView(device, iv, nullptr);
        swapChainImageViews.clear();
        if (swapChain) { vkDestroySwapchainKHR(device, swapChain, nullptr); swapChain = VK_NULL_HANDLE; }
        if (depthImageView) { vkDestroyImageView(device, depthImageView, nullptr); depthImageView = VK_NULL_HANDLE; }
        if (depthImage) { vkDestroyImage(device, depthImage, nullptr); depthImage = VK_NULL_HANDLE; }
        if (depthImageMemory) { vkFreeMemory(device, depthImageMemory, nullptr); depthImageMemory = VK_NULL_HANDLE; }

        for (size_t i = 0; i < uniformBuffers.size(); i++) {
            if (uniformBuffers[i]) vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            if (uniformBuffersMemory[i]) vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }
        uniformBuffers.clear();
        uniformBuffersMemory.clear();
        uniformBuffersMapped.clear();

        if (descriptorPool) { vkDestroyDescriptorPool(device, descriptorPool, nullptr); descriptorPool = VK_NULL_HANDLE; }
        descriptorSets.clear();
    }

    void ModelViewer::recreateSwapChain() {
        vkDeviceWaitIdle(device);
        cleanupSwapChain();
        createSwapChain();
        createImageViews();
        createDepthResources();
        createRenderPass();
        createGraphicsPipelines();
        createFramebuffers();
        createDescriptorPool();
        createUniformBuffers();
        createDescriptorSets();
        createCommandBuffers();
    }

    void ModelViewer::cleanup() {
        vkDeviceWaitIdle(device);
        cleanupSwapChain();

        vkDestroySampler(device, textureSampler, nullptr);
        
        for (auto &t : textures) {
            if (t.view) vkDestroyImageView(device, t.view, nullptr);
            if (t.image) vkDestroyImage(device, t.image, nullptr);
            if (t.memory) vkFreeMemory(device, t.memory, nullptr);
        }
        textures.clear();
        textureImage = VK_NULL_HANDLE;
        textureImageView = VK_NULL_HANDLE;
        textureImageMemory = VK_NULL_HANDLE;

        model.cleanup(device);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        if (commandPool) vkDestroyCommandPool(device, commandPool, nullptr);
        if (device) vkDestroyDevice(device, nullptr);
        if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
        if (instance) vkDestroyInstance(instance, nullptr);
#ifndef WITH_MOLTEN
        volkFinalize();
#endif
        if (window) { SDL_DestroyWindow(window); SDL_Quit(); }
    }

}
