#include"vk_abstract_model.hpp"
#include"loadpng.hpp"

namespace mx {

    void VKAbstractModel::load(VKWindow *win, const std::string &model_file, const std::string &texture_file, const std::string &path, float scale) {
        obj.load(model_file, scale);
        const auto &verts = obj.vertices();
        if (!verts.empty()) {
            float minX = verts[0].pos[0], maxX = minX;
            float minY = verts[0].pos[1], maxY = minY;
            float minZ = verts[0].pos[2], maxZ = minZ;
            for (const auto &v : verts) {
                minX = std::min(minX, v.pos[0]); maxX = std::max(maxX, v.pos[0]);
                minY = std::min(minY, v.pos[1]); maxY = std::max(maxY, v.pos[1]);
                minZ = std::min(minZ, v.pos[2]); maxZ = std::max(maxZ, v.pos[2]);
            }
            modelCenterOffset = glm::vec3(
                -0.5f * (minX + maxX),
                -0.5f * (minY + maxY),
                -0.5f * (minZ + maxZ));
            float maxExtent = std::max(maxX - minX, std::max(maxY - minY, maxZ - minZ));
            if (maxExtent > 1e-6f)
                modelRenderScale = 2.5f / maxExtent;
        }

        obj.upload(win->getDevice(), win->getPhysicalDevice(), win->getCommandPool(), win->getGraphicsQueue());
        loadModelTextures(win, texture_file, path);
        const size_t frameCount = win->swapChainImages.size();
        modelUniformBuffers.resize(frameCount);
        modelUniformBufferMemory.resize(frameCount);
        modelUniformBuffersMapped.resize(frameCount, nullptr);
        for (size_t i = 0; i < frameCount; ++i) {
            win->createBuffer(sizeof(mx::UniformBufferObject),
                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              modelUniformBuffers[i], modelUniformBufferMemory[i]);
            vkMapMemory(win->getDevice(), modelUniformBufferMemory[i], 0,
                        sizeof(mx::UniformBufferObject), 0, &modelUniformBuffersMapped[i]);
        }

        createModelDescriptorPool(win);
        createModelDescriptorSets(win);
    }
    
    void VKAbstractModel::setShaders(const std::string &vert, const std::string &frag) {

    }
    
    void VKAbstractModel::render(int cmd) {
        
    }

    void VKAbstractModel::updateUBO(VKWindow *win, uint32_t imageIndex, const mx::UniformBufferObject &ubo) {
        if (imageIndex < modelUniformBuffersMapped.size() && modelUniformBuffersMapped[imageIndex])
            memcpy(modelUniformBuffersMapped[imageIndex], &ubo, sizeof(ubo));
    }

    void VKAbstractModel::loadModelTextures(VKWindow *win, const std::string &texture_file, const std::string &path) {
        std::string texPath = texture_file;
        std::ifstream tf(texPath);
        if (!tf.is_open())
            throw mx::Exception("Failed to open .tex file: " + texPath);

        std::string prefix = path;
        std::vector<std::string> imagePaths;
        std::string line;
        while (std::getline(tf, line)) {
            size_t b = line.find_first_not_of(" \t\r\n");
            if (b == std::string::npos) continue;
            size_t e = line.find_last_not_of(" \t\r\n");
            line = line.substr(b, e - b + 1);
            if (line.empty() || line[0] == '#') continue;
            imagePaths.push_back(prefix + "/" + line);
        }
        if (imagePaths.empty())
            throw mx::Exception("No textures in .tex file");

        VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
        for (size_t i = 0; i < imagePaths.size(); ++i) {
            SDL_Surface *img = png::LoadPNG(imagePaths[i].c_str());
            if (!img) {
                img = SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 32, SDL_PIXELFORMAT_RGBA32);
                if (img) { uint32_t *px = (uint32_t*)img->pixels; *px = 0xFFFFFFFF; }
                else throw mx::Exception("Failed to create placeholder surface");
            }

            TexEntry tex;
            tex.w = img->w;
            tex.h = img->h;
            VkDeviceSize imageSize = tex.w * tex.h * 4;

            win->createImage(tex.w, tex.h, fmt, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex.image, tex.memory);

            win->transitionImageLayout(tex.image, fmt, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkBuffer staging; VkDeviceMemory stagingMem;
            win->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         staging, stagingMem);
            void *data;
            vkMapMemory(win->getDevice(), stagingMem, 0, imageSize, 0, &data);
            memcpy(data, img->pixels, (size_t)imageSize);
            vkUnmapMemory(win->getDevice(), stagingMem);
            win->copyBufferToImage(staging, tex.image, tex.w, tex.h);

            win->transitionImageLayout(tex.image, fmt, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            vkDestroyBuffer(win->getDevice(), staging, nullptr);
            vkFreeMemory(win->getDevice(), stagingMem, nullptr);
            SDL_FreeSurface(img);

            tex.view = win->createImageView(tex.image, fmt, VK_IMAGE_ASPECT_COLOR_BIT);
            modelTextures.push_back(tex);
        }
    }
    
    void VKAbstractModel::resize(VKWindow *win) {
        if (modelDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(win->getDevice(), modelDescriptorPool, nullptr);
            modelDescriptorPool = VK_NULL_HANDLE;
        }
        modelDescriptorSets.clear();
        // Reallocate per-model UBOs for the new swap chain size
        for (size_t i = 0; i < modelUniformBuffers.size(); ++i) {
            if (modelUniformBuffersMapped[i])
                vkUnmapMemory(win->getDevice(), modelUniformBufferMemory[i]);
            if (modelUniformBuffers[i]      != VK_NULL_HANDLE) vkDestroyBuffer(win->getDevice(), modelUniformBuffers[i], nullptr);
            if (modelUniformBufferMemory[i] != VK_NULL_HANDLE) vkFreeMemory(win->getDevice(), modelUniformBufferMemory[i], nullptr);
        }
        const size_t frameCount = win->swapChainImages.size();
        modelUniformBuffers.assign(frameCount, VK_NULL_HANDLE);
        modelUniformBufferMemory.assign(frameCount, VK_NULL_HANDLE);
        modelUniformBuffersMapped.assign(frameCount, nullptr);
        for (size_t i = 0; i < frameCount; ++i) {
            win->createBuffer(sizeof(mx::UniformBufferObject),
                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              modelUniformBuffers[i], modelUniformBufferMemory[i]);
            vkMapMemory(win->getDevice(), modelUniformBufferMemory[i], 0,
                        sizeof(mx::UniformBufferObject), 0, &modelUniformBuffersMapped[i]);
        }
        createModelDescriptorPool(win);
        createModelDescriptorSets(win);
    }

    void VKAbstractModel::createModelDescriptorPool(VKWindow *win) {
        const uint32_t texCount = std::max<uint32_t>(1u, static_cast<uint32_t>(modelTextures.size()));
        const uint32_t setCount = static_cast<uint32_t>(win->swapChainImages.size()) * texCount;

        std::array<VkDescriptorPoolSize, 2> sizes{};
        sizes[0] = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, setCount};
        sizes[1] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, setCount};

        VkDescriptorPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ci.poolSizeCount = static_cast<uint32_t>(sizes.size());
        ci.pPoolSizes = sizes.data();
        ci.maxSets = setCount;
        if (vkCreateDescriptorPool(win->getDevice(), &ci, nullptr, &modelDescriptorPool) != VK_SUCCESS)
            throw mx::Exception("Failed to create model descriptor pool!");
    }
    void VKAbstractModel::createModelDescriptorSets(VKWindow *win) {
        const size_t texCount = std::max<size_t>(1, modelTextures.size());
        const size_t setCount = win->swapChainImages.size() * texCount;

        std::vector<VkDescriptorSetLayout> layouts(setCount, win->descriptorSetLayout);
        VkDescriptorSetAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool = modelDescriptorPool;
        ai.descriptorSetCount = static_cast<uint32_t>(setCount);
        ai.pSetLayouts = layouts.data();
        modelDescriptorSets.resize(setCount);
        if (vkAllocateDescriptorSets(win->getDevice(), &ai, modelDescriptorSets.data()) != VK_SUCCESS)
            throw mx::Exception("Failed to allocate model descriptor sets!");

        for (size_t frame = 0; frame < win->swapChainImages.size(); ++frame) {
            VkDescriptorBufferInfo bufInfo{};
            bufInfo.buffer = modelUniformBuffers[frame];  // use per-model UBO
            bufInfo.offset = 0;
            bufInfo.range  = sizeof(mx::UniformBufferObject);

            for (size_t tex = 0; tex < texCount; ++tex) {
                size_t setIndex = frame * texCount + tex;

                VkDescriptorImageInfo imgInfo{};
                imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imgInfo.imageView   = modelTextures[tex].view;
                imgInfo.sampler     = win->textureSampler;

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

                vkUpdateDescriptorSets(win->getDevice(), static_cast<uint32_t>(writes.size()),
                                       writes.data(), 0, nullptr);
            }
        }
    }

    void VKAbstractModel::cleanup(VKWindow *win) {
        vkDeviceWaitIdle(win->getDevice());
        obj.cleanup(win->getDevice());
        for (size_t i = 0; i < modelUniformBuffers.size(); ++i) {
            if (modelUniformBuffersMapped[i])
                vkUnmapMemory(win->getDevice(), modelUniformBufferMemory[i]);
            if (modelUniformBuffers[i]   != VK_NULL_HANDLE) vkDestroyBuffer(win->getDevice(), modelUniformBuffers[i], nullptr);
            if (modelUniformBufferMemory[i] != VK_NULL_HANDLE) vkFreeMemory(win->getDevice(), modelUniformBufferMemory[i], nullptr);
        }
        modelUniformBuffers.clear();
        modelUniformBufferMemory.clear();
        modelUniformBuffersMapped.clear();
        for (auto &t : modelTextures) {
            if (t.view   != VK_NULL_HANDLE) vkDestroyImageView(win->getDevice(), t.view, nullptr);
            if (t.image  != VK_NULL_HANDLE) vkDestroyImage(win->getDevice(), t.image, nullptr);
            if (t.memory != VK_NULL_HANDLE) vkFreeMemory(win->getDevice(), t.memory, nullptr);
        }
        modelTextures.clear();
        if (modelDescriptorPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(win->getDevice(), modelDescriptorPool, nullptr);
        
    }
}