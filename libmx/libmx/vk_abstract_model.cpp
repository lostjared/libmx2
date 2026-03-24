#include "vk_abstract_model.hpp"
#include "loadpng.hpp"

namespace mx {

    void VKAbstractModel::load(VKWindow *win, const std::string &model_file, const std::string &texture_file, const std::string &path, float scale) {
        std::cout << ">> VKAbstractModel::load: model=" << model_file << ", textures=" << texture_file << ", scale=" << scale << "\n";
        obj.load(model_file, scale);
        const auto &verts = obj.vertices();
        if (!verts.empty()) {
            float minX = verts[0].pos[0], maxX = minX;
            float minY = verts[0].pos[1], maxY = minY;
            float minZ = verts[0].pos[2], maxZ = minZ;
            for (const auto &v : verts) {
                minX = std::min(minX, v.pos[0]);
                maxX = std::max(maxX, v.pos[0]);
                minY = std::min(minY, v.pos[1]);
                maxY = std::max(maxY, v.pos[1]);
                minZ = std::min(minZ, v.pos[2]);
                maxZ = std::max(maxZ, v.pos[2]);
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
        std::cout << ">> VKAbstractModel::load: model uploaded, center offset=(" << modelCenterOffset.x << ", " << modelCenterOffset.y << ", " << modelCenterOffset.z << "), renderScale=" << modelRenderScale << "\n";
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
        std::cout << ">> VKAbstractModel::load: complete [OK]\n";
    }

    void VKAbstractModel::loadInstance(VKWindow *win, VKAbstractModel &source) {
        std::cout << ">> VKAbstractModel::loadInstance: creating lightweight instance\n";
        isInstance_ = true;
        modelCenterOffset = source.modelCenterOffset;
        modelRenderScale = source.modelRenderScale;
        // Share texture entries (source owns the GPU resources)
        modelTextures = source.modelTextures;

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
        std::cout << ">> VKAbstractModel::loadInstance: complete [OK]\n";
    }

    void VKAbstractModel::cleanupInstanceResources(VKWindow *win) {
        for (size_t i = 0; i < modelUniformBuffers.size(); ++i) {
            if (modelUniformBuffersMapped[i])
                vkUnmapMemory(win->getDevice(), modelUniformBufferMemory[i]);
            if (modelUniformBuffers[i] != VK_NULL_HANDLE)
                vkDestroyBuffer(win->getDevice(), modelUniformBuffers[i], nullptr);
            if (modelUniformBufferMemory[i] != VK_NULL_HANDLE)
                vkFreeMemory(win->getDevice(), modelUniformBufferMemory[i], nullptr);
        }
        modelUniformBuffers.clear();
        modelUniformBufferMemory.clear();
        modelUniformBuffersMapped.clear();
        if (modelDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(win->getDevice(), modelDescriptorPool, nullptr);
            modelDescriptorPool = VK_NULL_HANDLE;
        }
        modelDescriptorSets.clear();
        // Don't destroy textures or mesh — source model owns them
        modelTextures.clear();
    }

    void VKAbstractModel::setShaders(VKWindow *win, const std::string &vert, const std::string &frag) {
        vertShaderPath_ = vert;
        fragShaderPath_ = frag;
        if (modelPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(win->getDevice(), modelPipeline, nullptr);
            modelPipeline = VK_NULL_HANDLE;
        }
        if (modelPipelineWireframe != VK_NULL_HANDLE) {
            vkDestroyPipeline(win->getDevice(), modelPipelineWireframe, nullptr);
            modelPipelineWireframe = VK_NULL_HANDLE;
        }
        createModelPipeline(win);
        std::cout << ">> VKAbstractModel::setShaders: vert=" << vert << ", frag=" << frag << " [OK]\n";
    }

    void VKAbstractModel::render(int cmd) {
    }

    void VKAbstractModel::updateUBO(VKWindow *win, uint32_t imageIndex, const mx::UniformBufferObject &ubo) {
        if (imageIndex < modelUniformBuffersMapped.size() && modelUniformBuffersMapped[imageIndex])
            memcpy(modelUniformBuffersMapped[imageIndex], &ubo, sizeof(ubo));
    }

    void VKAbstractModel::loadModelTextures(VKWindow *win, const std::string &texture_file, const std::string &path) {
        std::cout << ">> VKAbstractModel::loadModelTextures: loading from " << texture_file << "\n";
        std::string texPath = texture_file;
        std::ifstream tf(texPath);
        if (!tf.is_open())
            throw mx::Exception("Failed to open .tex file: " + texPath);

        std::string prefix = path;
        std::vector<std::string> imagePaths;

        // Read all meaningful lines to detect format
        std::vector<std::string> lines;
        {
            std::string line;
            while (std::getline(tf, line)) {
                size_t b = line.find_first_not_of(" \t\r\n");
                if (b == std::string::npos)
                    continue;
                size_t e = line.find_last_not_of(" \t\r\n");
                line = line.substr(b, e - b + 1);
                if (line.empty() || line[0] == '#')
                    continue;
                lines.push_back(line);
            }
        }

        // Detect structured format by looking for submesh keyword
        bool structured = false;
        bool isMTL = false;
        for (const auto &ln : lines) {
            std::istringstream chk(ln);
            std::string kw;
            chk >> kw;
            if (kw == "submesh" || kw == "texture_dir" || kw == "material_lib" || kw == "model") {
                structured = true;
                break;
            }
            if (kw == "newmtl") {
                isMTL = true;
                break;
            }
        }

        if (isMTL) {
            std::cout << ">> VKAbstractModel::loadModelTextures: MTL format detected, extracting map_Kd textures\n";
            for (const auto &ln : lines) {
                std::istringstream s(ln);
                std::string kw;
                s >> kw;
                if (kw == "map_Kd") {
                    std::string texFile;
                    if (s >> texFile)
                        imagePaths.push_back(prefix + "/" + texFile);
                }
            }
        } else if (structured) {
            std::cout << ">> VKAbstractModel::loadModelTextures: structured .tex format detected\n";
            for (const auto &ln : lines) {
                std::istringstream s(ln);
                std::string kw;
                s >> kw;
                if (kw == "texture") {
                    std::string texFile;
                    if (s >> texFile)
                        imagePaths.push_back(prefix + "/" + texFile);
                }
                // texture_dir, material_lib, model, submesh, end_submesh,
                // diffuse, specular_power, smooth_group, size, material, object
                // are parsed but only 'texture' produces an image path
            }
        } else {
            // Plain format: one filename per line
            for (const auto &ln : lines)
                imagePaths.push_back(prefix + "/" + ln);
        }

        if (imagePaths.empty())
            throw mx::Exception("No textures in .tex file");

        VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
        for (size_t i = 0; i < imagePaths.size(); ++i) {
            SDL_Surface *img = png::LoadPNG(imagePaths[i].c_str());
            if (!img) {
                img = SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 32, SDL_PIXELFORMAT_RGBA32);
                if (img) {
                    uint32_t *px = (uint32_t *)img->pixels;
                    *px = 0xFFFFFFFF;
                } else
                    throw mx::Exception("Failed to create placeholder surface");
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

            VkBuffer staging;
            VkDeviceMemory stagingMem;
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
            std::cout << ">> VKAbstractModel::loadModelTextures: [" << i << "] " << imagePaths[i] << " (" << tex.w << "x" << tex.h << ")\n";
        }
        std::cout << ">> VKAbstractModel::loadModelTextures: " << modelTextures.size() << " texture(s) loaded [OK]\n";
    }

    void VKAbstractModel::loadModelTexturesFromMTL(VKWindow *win, const std::string &path) {
        const auto &mats = obj.materials();
        if (mats.empty()) {
            std::cerr << ">> VKAbstractModel::loadModelTexturesFromMTL: no materials loaded\n";
            return;
        }
        std::cout << ">> VKAbstractModel::loadModelTexturesFromMTL: loading " << mats.size() << " texture(s) from MTL map_Kd\n";
        std::string prefix = path;
        // Resolve mtl directory from the mtl file path
        std::string mtlDir;
        if (!obj.mtlLibPath().empty()) {
            auto pos = obj.mtlLibPath().find_last_of("/\\");
            if (pos != std::string::npos)
                mtlDir = obj.mtlLibPath().substr(0, pos);
        }

        VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
        for (size_t i = 0; i < mats.size(); ++i) {
            std::string imgPath;
            if (!mats[i].map_kd.empty()) {
                // Try prefix first, then mtl directory
                imgPath = prefix + "/" + mats[i].map_kd;
            }

            SDL_Surface *img = nullptr;
            if (!imgPath.empty())
                img = png::LoadPNG(imgPath.c_str());

            if (!img) {
                // Create a solid-colour placeholder from Kd
                img = SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 32, SDL_PIXELFORMAT_RGBA32);
                if (img) {
                    uint8_t r = static_cast<uint8_t>(std::min(1.0f, mats[i].kd[0]) * 255.0f);
                    uint8_t g = static_cast<uint8_t>(std::min(1.0f, mats[i].kd[1]) * 255.0f);
                    uint8_t b = static_cast<uint8_t>(std::min(1.0f, mats[i].kd[2]) * 255.0f);
                    uint32_t *px = (uint32_t *)img->pixels;
                    *px = (0xFF << 24) | (b << 16) | (g << 8) | r;
                } else {
                    throw mx::Exception("Failed to create placeholder surface for material " + mats[i].name);
                }
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

            VkBuffer staging;
            VkDeviceMemory stagingMem;
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
            std::cout << ">> VKAbstractModel::loadModelTexturesFromMTL: [" << i << "] "
                      << mats[i].name << " -> " << (imgPath.empty() ? "(Kd placeholder)" : imgPath)
                      << " (" << tex.w << "x" << tex.h << ")\n";
        }
        std::cout << ">> VKAbstractModel::loadModelTexturesFromMTL: " << modelTextures.size() << " texture(s) loaded [OK]\n";
    }

    void VKAbstractModel::resize(VKWindow *win) {
        std::cout << ">> VKAbstractModel::resize: rebuilding UBOs and descriptors for new swapchain\n";
        if (modelDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(win->getDevice(), modelDescriptorPool, nullptr);
            modelDescriptorPool = VK_NULL_HANDLE;
        }
        modelDescriptorSets.clear();
        for (size_t i = 0; i < modelUniformBuffers.size(); ++i) {
            if (modelUniformBuffersMapped[i])
                vkUnmapMemory(win->getDevice(), modelUniformBufferMemory[i]);
            if (modelUniformBuffers[i] != VK_NULL_HANDLE)
                vkDestroyBuffer(win->getDevice(), modelUniformBuffers[i], nullptr);
            if (modelUniformBufferMemory[i] != VK_NULL_HANDLE)
                vkFreeMemory(win->getDevice(), modelUniformBufferMemory[i], nullptr);
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
        if (!vertShaderPath_.empty())
            createModelPipeline(win);
        std::cout << ">> VKAbstractModel::resize: complete (" << win->swapChainImages.size() << " frames) [OK]\n";
    }

    void VKAbstractModel::createModelDescriptorPool(VKWindow *win) {
        std::cout << ">> VKAbstractModel::createModelDescriptorPool: creating pool...\n";
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
        std::cout << ">> VKAbstractModel::createModelDescriptorPool: maxSets=" << setCount << " [OK]\n";
    }
    void VKAbstractModel::createModelDescriptorSets(VKWindow *win) {
        std::cout << ">> VKAbstractModel::createModelDescriptorSets: allocating sets...\n";
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
            bufInfo.buffer = modelUniformBuffers[frame]; // use per-model UBO
            bufInfo.offset = 0;
            bufInfo.range = sizeof(mx::UniformBufferObject);

            for (size_t tex = 0; tex < texCount; ++tex) {
                size_t setIndex = frame * texCount + tex;

                VkDescriptorImageInfo imgInfo{};
                imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imgInfo.imageView = modelTextures[tex].view;
                imgInfo.sampler = win->textureSampler;

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
        std::cout << ">> VKAbstractModel::createModelDescriptorSets: " << setCount << " set(s) allocated [OK]\n";
    }

    void VKAbstractModel::cleanup(VKWindow *win) {
        std::cout << ">> VKAbstractModel::cleanup: releasing model resources..."
                  << (isInstance_ ? " (instance)" : "") << "\n";
        vkDeviceWaitIdle(win->getDevice());
        if (isInstance_) {
            cleanupInstanceResources(win);
            if (modelPipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(win->getDevice(), modelPipeline, nullptr);
                modelPipeline = VK_NULL_HANDLE;
            }
            if (modelPipelineWireframe != VK_NULL_HANDLE) {
                vkDestroyPipeline(win->getDevice(), modelPipelineWireframe, nullptr);
                modelPipelineWireframe = VK_NULL_HANDLE;
            }
            std::cout << ">> VKAbstractModel::cleanup: instance resources released [OK]\n";
            return;
        }
        if (modelPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(win->getDevice(), modelPipeline, nullptr);
            modelPipeline = VK_NULL_HANDLE;
        }
        if (modelPipelineWireframe != VK_NULL_HANDLE) {
            vkDestroyPipeline(win->getDevice(), modelPipelineWireframe, nullptr);
            modelPipelineWireframe = VK_NULL_HANDLE;
        }
        obj.cleanup(win->getDevice());
        for (size_t i = 0; i < modelUniformBuffers.size(); ++i) {
            if (modelUniformBuffersMapped[i])
                vkUnmapMemory(win->getDevice(), modelUniformBufferMemory[i]);
            if (modelUniformBuffers[i] != VK_NULL_HANDLE)
                vkDestroyBuffer(win->getDevice(), modelUniformBuffers[i], nullptr);
            if (modelUniformBufferMemory[i] != VK_NULL_HANDLE)
                vkFreeMemory(win->getDevice(), modelUniformBufferMemory[i], nullptr);
        }
        modelUniformBuffers.clear();
        modelUniformBufferMemory.clear();
        modelUniformBuffersMapped.clear();
        for (auto &t : modelTextures) {
            if (t.view != VK_NULL_HANDLE)
                vkDestroyImageView(win->getDevice(), t.view, nullptr);
            if (t.image != VK_NULL_HANDLE)
                vkDestroyImage(win->getDevice(), t.image, nullptr);
            if (t.memory != VK_NULL_HANDLE)
                vkFreeMemory(win->getDevice(), t.memory, nullptr);
        }
        modelTextures.clear();
        if (modelDescriptorPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(win->getDevice(), modelDescriptorPool, nullptr);
        std::cout << ">> VKAbstractModel::cleanup: all resources released [OK]\n";
    }

    void VKAbstractModel::createModelPipeline(VKWindow *win) {
        if (vertShaderPath_.empty() || fragShaderPath_.empty())
            return;

        if (modelPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(win->getDevice(), modelPipeline, nullptr);
            modelPipeline = VK_NULL_HANDLE;
        }
        if (modelPipelineWireframe != VK_NULL_HANDLE) {
            vkDestroyPipeline(win->getDevice(), modelPipelineWireframe, nullptr);
            modelPipelineWireframe = VK_NULL_HANDLE;
        }

        std::cout << ">> VKAbstractModel::createModelPipeline: creating pipeline...\n";

        auto vertCode = mx::readFile(vertShaderPath_);
        auto fragCode = mx::readFile(fragShaderPath_);

        VkShaderModule vertModule = win->createShaderModule(vertCode);
        VkShaderModule fragModule = win->createShaderModule(fragCode);

        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertModule;
        vertStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragStage{};
        fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = fragModule;
        fragStage.pName = "main";

        VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(Vertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 3> attrs{};
        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = offsetof(Vertex, pos);
        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[1].offset = offsetof(Vertex, texCoord);
        attrs[2].binding = 0;
        attrs[2].location = 2;
        attrs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[2].offset = offsetof(Vertex, normal);

        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &binding;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
        vertexInput.pVertexAttributeDescriptions = attrs.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        std::array<VkDynamicState, 2> dynStates = {
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynState{};
        dynState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynState.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
        dynState.pDynamicStates = dynStates.data();

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkGraphicsPipelineCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        ci.stageCount = 2;
        ci.pStages = stages;
        ci.pVertexInputState = &vertexInput;
        ci.pInputAssemblyState = &inputAssembly;
        ci.pViewportState = &viewportState;
        ci.pRasterizationState = &rasterizer;
        ci.pMultisampleState = &multisampling;
        ci.pDepthStencilState = &depthStencil;
        ci.pColorBlendState = &colorBlending;
        ci.pDynamicState = &dynState;
        ci.layout = win->pipelineLayout;
        ci.renderPass = win->renderPass;
        ci.subpass = 0;

        if (vkCreateGraphicsPipelines(win->getDevice(), VK_NULL_HANDLE, 1, &ci, nullptr, &modelPipeline) != VK_SUCCESS)
            throw mx::Exception("VKAbstractModel: Failed to create model pipeline!");

        rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
        if (vkCreateGraphicsPipelines(win->getDevice(), VK_NULL_HANDLE, 1, &ci, nullptr, &modelPipelineWireframe) != VK_SUCCESS) {
            modelPipelineWireframe = VK_NULL_HANDLE;
        }

        vkDestroyShaderModule(win->getDevice(), fragModule, nullptr);
        vkDestroyShaderModule(win->getDevice(), vertModule, nullptr);

        std::cout << ">> VKAbstractModel::createModelPipeline: pipeline created [OK]\n";
    }
} // namespace mx