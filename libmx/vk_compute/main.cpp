#include "argz.hpp"
#include "vk.hpp"
#include "vk_cv.hpp"
#include <opencv2/opencv.hpp>

#include <array>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <vector>

static constexpr int HISTORY_SIZE = 8;

struct ComputePC {
    int32_t mode;
    int32_t historyCount;
    int32_t historyIdx;
    int32_t square_size;
    int32_t history_dir;
};

class ComputeWindow : public mx::VKWindow {
  public:
    ComputeWindow(const std::string &path, int wx, int wy, bool full, int camIdx)
        : mx::VKWindow("-[ VK Compute CV ]-", wx, wy, full), cameraIndex(camIdx) {
        setPath(path);
    }

    void loadSPV() {
        std::ifstream f(util.path + "/index.txt");
        if (!f.is_open())
            throw mx::Exception("Cannot open: " + util.path + "/index.txt");
        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty())
                spvFiles.push_back(line);
        }
        if (spvFiles.empty())
            throw mx::Exception("index.txt contains no entries");
    }

    void initVulkan() override {
        mx::VKWindow::initVulkan();
        initComputeResources();
    }

    bool shouldRender3D() override { return false; }

    void proc() override {
        cv::Mat frame;
        if (capture.read(frame) && !frame.empty()) {
            cv::Mat rgba;
            if (frame.channels() == 4)
                cv::cvtColor(frame, rgba, cv::COLOR_BGRA2RGBA);
            else if (frame.channels() == 3)
                cv::cvtColor(frame, rgba, cv::COLOR_BGR2RGBA);
            else if (frame.channels() == 1)
                cv::cvtColor(frame, rgba, cv::COLOR_GRAY2RGBA);

            if (!rgba.empty() && rgba.cols == texWidth && rgba.rows == texHeight) {
                uploadToImage(rgba.ptr(), workImg[0]);
                tickAnimState();
                runComputeFrame();
            }
        }

        if (displaySprite)
            displaySprite->drawSpriteRect(0, 0, getWidth(), getHeight());
    }

    void event(SDL_Event &e) override {
        if (e.type == SDL_QUIT ||
            (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
            quit();
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_UP && !spvFiles.empty()) {
                currentSpvIndex = (currentSpvIndex - 1 + static_cast<int>(spvFiles.size())) % static_cast<int>(spvFiles.size());
                reloadPipeline();
            } else if (e.key.keysym.sym == SDLK_DOWN && !spvFiles.empty()) {
                currentSpvIndex = (currentSpvIndex + 1) % static_cast<int>(spvFiles.size());
                reloadPipeline();
            }
        }
    }

    void cleanup() override {
        capture.close();
        destroyComputeResources();
        mx::VKWindow::cleanup();
    }

  private:
    mx::MXCapture capture;
    int cameraIndex = 0;
    int texWidth = 1920;
    int texHeight = 1080;

    struct CImg {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };

    std::array<CImg, 2> workImg;
    std::array<CImg, HISTORY_SIZE> histImg;
    CImg outImg;

    VkSampler computeSampler = VK_NULL_HANDLE;

    VkBuffer stagingBuf = VK_NULL_HANDLE;
    VkDeviceMemory stagingMem = VK_NULL_HANDLE;
    VkBuffer readbackBuf = VK_NULL_HANDLE;
    VkDeviceMemory readbackMem = VK_NULL_HANDLE;
    void *readbackMap = nullptr;

    VkDescriptorSetLayout compDSLayout = VK_NULL_HANDLE;
    VkPipelineLayout compPipeLayout = VK_NULL_HANDLE;
    VkPipeline compPipeline = VK_NULL_HANDLE;
    VkDescriptorPool compDSPool = VK_NULL_HANDLE;

    VkDescriptorSet blurDS[2] = {};
    VkDescriptorSet blendDS[2] = {};

    mx::VKSprite *displaySprite = nullptr;

    int historyIndex = 0;
    int historyCount = 0;
    int current_square = 4;
    int square_dir = 1;
    int current_histIdx = 0;
    int current_dir = 1;

    std::vector<std::string> spvFiles;
    int currentSpvIndex = 0;

    void initComputeResources() {
        if (!capture.open(cameraIndex))
            throw mx::Exception("Failed to open camera " + std::to_string(cameraIndex));

        capture.set(cv::CAP_PROP_FRAME_WIDTH, 1920.0);
        capture.set(cv::CAP_PROP_FRAME_HEIGHT, 1080.0);
        capture.set(cv::CAP_PROP_FPS, 60.0);

        cv::Mat frame;
        if (!capture.read(frame) || frame.empty())
            throw mx::Exception("Failed to read initial camera frame");

        texWidth = frame.cols;
        texHeight = frame.rows;

        const VkDeviceSize imgBytes =
            static_cast<VkDeviceSize>(texWidth) * texHeight * 4;

        createBuffer(imgBytes,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuf, stagingMem);

        createBuffer(imgBytes,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     readbackBuf, readbackMem);
        vkMapMemory(device, readbackMem, 0, imgBytes, 0, &readbackMap);

        {
            auto cmd = beginSingleTimeCommands();
            allocCImg(workImg[0], cmd);
            allocCImg(workImg[1], cmd);
            for (auto &img : histImg)
                allocCImg(img, cmd);
            allocCImg(outImg, cmd);
            endSingleTimeCommands(cmd);
        }

        VkSamplerCreateInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        si.magFilter = VK_FILTER_LINEAR;
        si.minFilter = VK_FILTER_LINEAR;
        si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        si.maxAnisotropy = 1.0f;
        VK_CHECK_RESULT(vkCreateSampler(device, &si, nullptr, &computeSampler));

        loadSPV();
        buildDescriptorSetLayout();
        buildComputePipeline();
        buildDescriptorSets();

        displaySprite = createSprite(texWidth, texHeight,
                                     util.path + "/data/sprite_vert.spv",
                                     util.path + "/data/sprite_frag.spv");
    }

    void allocCImg(CImg &img, VkCommandBuffer cmd) {
        createImage(
            static_cast<uint32_t>(texWidth),
            static_cast<uint32_t>(texHeight),
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            img.image, img.memory);
        img.view = createImageView(
            img.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

        VkImageMemoryBarrier b{};
        b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        b.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = img.image;
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        b.srcAccessMask = 0;
        b.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    void reloadPipeline() {
        vkDeviceWaitIdle(device);
        if (compPipeline)   { vkDestroyPipeline(device, compPipeline, nullptr);           compPipeline   = VK_NULL_HANDLE; }
        if (compPipeLayout) { vkDestroyPipelineLayout(device, compPipeLayout, nullptr);   compPipeLayout = VK_NULL_HANDLE; }
        buildComputePipeline();
    }

    void buildDescriptorSetLayout() {
        std::array<VkDescriptorSetLayoutBinding, 3> bindings{};

        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[2].descriptorCount = HISTORY_SIZE;
        bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ci.bindingCount = static_cast<uint32_t>(bindings.size());
        ci.pBindings = bindings.data();
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &ci, nullptr, &compDSLayout));
    }

    void buildComputePipeline() {
        const std::string spvPath = util.path + "/" + spvFiles[currentSpvIndex];
        std::ifstream spvFile(spvPath, std::ios::binary | std::ios::ate);
        if (!spvFile.is_open())
            throw mx::Exception("Cannot open compute SPIR-V: " + spvPath);
        const size_t sz = static_cast<size_t>(spvFile.tellg());
        std::vector<char> spv(sz);
        spvFile.seekg(0);
        spvFile.read(spv.data(), static_cast<std::streamsize>(sz));

        VkShaderModuleCreateInfo mci{};
        mci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        mci.codeSize = spv.size();
        mci.pCode = reinterpret_cast<const uint32_t *>(spv.data());
        VkShaderModule mod;
        VK_CHECK_RESULT(vkCreateShaderModule(device, &mci, nullptr, &mod));

        VkPushConstantRange pcr{};
        pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pcr.offset = 0;
        pcr.size = sizeof(ComputePC);

        VkPipelineLayoutCreateInfo pli{};
        pli.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pli.setLayoutCount = 1;
        pli.pSetLayouts = &compDSLayout;
        pli.pushConstantRangeCount = 1;
        pli.pPushConstantRanges = &pcr;
        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pli, nullptr, &compPipeLayout));

        VkComputePipelineCreateInfo cpci{};
        cpci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        cpci.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        cpci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        cpci.stage.module = mod;
        cpci.stage.pName = "main";
        cpci.layout = compPipeLayout;
        VK_CHECK_RESULT(vkCreateComputePipelines(
            device, VK_NULL_HANDLE, 1, &cpci, nullptr, &compPipeline));

        vkDestroyShaderModule(device, mod, nullptr);
    }

    void buildDescriptorSets() {
        std::array<VkDescriptorPoolSize, 2> ps{};
        ps[0] = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4};
        ps[1] = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * (1 + HISTORY_SIZE)};

        VkDescriptorPoolCreateInfo dpi{};
        dpi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        dpi.maxSets = 4;
        dpi.poolSizeCount = static_cast<uint32_t>(ps.size());
        dpi.pPoolSizes = ps.data();
        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &dpi, nullptr, &compDSPool));
        std::array<VkDescriptorSetLayout, 4> layouts;
        layouts.fill(compDSLayout);
        VkDescriptorSetAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool = compDSPool;
        ai.descriptorSetCount = 4;
        ai.pSetLayouts = layouts.data();
        VkDescriptorSet raw[4];
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &ai, raw));
        blurDS[0] = raw[0];
        blurDS[1] = raw[1];
        blendDS[0] = raw[2];
        blendDS[1] = raw[3];
        writeBlurDS(blurDS[0], workImg[0].view, workImg[1].view);
        writeBlurDS(blurDS[1], workImg[1].view, workImg[0].view);
        writeBlendDS(blendDS[0], workImg[0].view);
        writeBlendDS(blendDS[1], workImg[1].view);
    }

    void writeBlurDS(VkDescriptorSet ds, VkImageView destV, VkImageView srcV) {
        VkDescriptorImageInfo di0{VK_NULL_HANDLE, destV, VK_IMAGE_LAYOUT_GENERAL};
        VkDescriptorImageInfo di1{computeSampler, srcV, VK_IMAGE_LAYOUT_GENERAL};
        std::vector<VkDescriptorImageInfo> hist(HISTORY_SIZE,
                                                VkDescriptorImageInfo{computeSampler, srcV, VK_IMAGE_LAYOUT_GENERAL});

        std::array<VkWriteDescriptorSet, 3> w{};
        w[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, ds, 0, 0, 1,
                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &di0, nullptr, nullptr};
        w[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, ds, 1, 0, 1,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &di1, nullptr, nullptr};
        w[2] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, ds, 2, 0,
                HISTORY_SIZE,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, hist.data(), nullptr, nullptr};
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(w.size()), w.data(), 0, nullptr);
    }

    void writeBlendDS(VkDescriptorSet ds, VkImageView srcV) {
        VkDescriptorImageInfo di0{VK_NULL_HANDLE, outImg.view, VK_IMAGE_LAYOUT_GENERAL};
        VkDescriptorImageInfo di1{computeSampler, srcV, VK_IMAGE_LAYOUT_GENERAL};

        std::vector<VkDescriptorImageInfo> hist(HISTORY_SIZE);
        for (int i = 0; i < HISTORY_SIZE; ++i)
            hist[i] = {computeSampler, histImg[i].view, VK_IMAGE_LAYOUT_GENERAL};

        std::array<VkWriteDescriptorSet, 3> w{};
        w[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, ds, 0, 0, 1,
                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &di0, nullptr, nullptr};
        w[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, ds, 1, 0, 1,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &di1, nullptr, nullptr};
        w[2] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, ds, 2, 0,
                HISTORY_SIZE,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, hist.data(), nullptr, nullptr};
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(w.size()), w.data(), 0, nullptr);
    }

    void uploadToImage(const void *data, CImg &img) {
        const VkDeviceSize bytes =
            static_cast<VkDeviceSize>(texWidth) * texHeight * 4;
        void *mapped;
        vkMapMemory(device, stagingMem, 0, bytes, 0, &mapped);
        memcpy(mapped, data, static_cast<size_t>(bytes));
        vkUnmapMemory(device, stagingMem);

        auto cmd = beginSingleTimeCommands();

        setLayout(cmd, img.image,
                  VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy r{};
        r.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        r.imageExtent = {static_cast<uint32_t>(texWidth),
                         static_cast<uint32_t>(texHeight), 1};
        vkCmdCopyBufferToImage(
            cmd, stagingBuf, img.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &r);

        setLayout(cmd, img.image,
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
                  VK_ACCESS_TRANSFER_WRITE_BIT,
                  VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        endSingleTimeCommands(cmd);
    }

    void dispatchOne(VkCommandBuffer cmd, VkDescriptorSet ds, int mode) {
        ComputePC pc{};
        pc.mode = mode;
        pc.historyCount = historyCount;
        pc.historyIdx = current_histIdx;
        pc.square_size = current_square;
        pc.history_dir = current_dir;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                                compPipeLayout, 0, 1, &ds, 0, nullptr);
        vkCmdPushConstants(cmd, compPipeLayout,
                           VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);

        const uint32_t gx = (static_cast<uint32_t>(texWidth) + 15) / 16;
        const uint32_t gy = (static_cast<uint32_t>(texHeight) + 15) / 16;
        vkCmdDispatch(cmd, gx, gy, 1);
    }

    void computeBarrier(VkCommandBuffer cmd, VkImage img) {
        VkImageMemoryBarrier b{};
        b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        b.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = img;
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        b.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    void runComputeFrame() {
        auto cmd = beginSingleTimeCommands();
        int srcIdx = 0, dstIdx = 1;
        const int passes = 3 + (std::rand() % 7);
        for (int i = 0; i < passes; ++i) {
            // blurDS[dstIdx]: dest=work[dstIdx], source=work[srcIdx]
            dispatchOne(cmd, blurDS[dstIdx], 0);
            computeBarrier(cmd, workImg[dstIdx].image);
            std::swap(srcIdx, dstIdx);
        }
        {
            std::array<VkImageMemoryBarrier, 2> bars{};
            bars[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            bars[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            bars[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            bars[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bars[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bars[0].image = workImg[srcIdx].image;
            bars[0].subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            bars[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            bars[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            bars[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            bars[1].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            bars[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            bars[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bars[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bars[1].image = histImg[historyIndex].image;
            bars[1].subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            bars[1].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            bars[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(cmd,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0, 0, nullptr, 0, nullptr,
                                 static_cast<uint32_t>(bars.size()), bars.data());

            VkImageCopy cp{};
            cp.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            cp.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            cp.extent = {static_cast<uint32_t>(texWidth),
                         static_cast<uint32_t>(texHeight), 1};
            vkCmdCopyImage(cmd,
                           workImg[srcIdx].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           histImg[historyIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &cp);

            bars[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            bars[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
            bars[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            bars[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

            bars[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            bars[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
            bars[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            bars[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 0, 0, nullptr, 0, nullptr,
                                 static_cast<uint32_t>(bars.size()), bars.data());
        }

        if (historyCount < HISTORY_SIZE)
            ++historyCount;
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;

        dispatchOne(cmd, blendDS[srcIdx], 1);

        {
            setLayout(cmd, outImg.image,
                      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            VkBufferImageCopy r{};
            r.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            r.imageExtent = {static_cast<uint32_t>(texWidth),
                             static_cast<uint32_t>(texHeight), 1};
            vkCmdCopyImageToBuffer(
                cmd, outImg.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                readbackBuf, 1, &r);

            setLayout(cmd, outImg.image,
                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
                      VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }

        endSingleTimeCommands(cmd);

        if (displaySprite)
            displaySprite->updateTexture(readbackMap, texWidth, texHeight);
    }

    void tickAnimState() {
        if (current_dir == 1) {
            if (++current_histIdx >= HISTORY_SIZE - 1) {
                current_histIdx = HISTORY_SIZE - 1;
                current_dir = -1;
            }
        } else {
            if (--current_histIdx <= 0) {
                current_histIdx = 0;
                current_dir = 1;
            }
        }

        if (square_dir == 1) {
            current_square += 2;
            if (current_square >= 64) {
                current_square = 64;
                square_dir = 0;
            }
        } else {
            current_square -= 2;
            if (current_square <= 2) {
                current_square = 2;
                square_dir = 1;
            }
        }
    }

    static void setLayout(VkCommandBuffer cmd, VkImage img,
                          VkImageLayout oldL, VkImageLayout newL,
                          VkAccessFlags srcA, VkAccessFlags dstA,
                          VkPipelineStageFlags srcS, VkPipelineStageFlags dstS) {
        VkImageMemoryBarrier b{};
        b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.oldLayout = oldL;
        b.newLayout = newL;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = img;
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        b.srcAccessMask = srcA;
        b.dstAccessMask = dstA;
        vkCmdPipelineBarrier(cmd, srcS, dstS, 0, 0, nullptr, 0, nullptr, 1, &b);
    }

    void destroyComputeResources() {
        vkDeviceWaitIdle(device);

        if (readbackMap) {
            vkUnmapMemory(device, readbackMem);
            readbackMap = nullptr;
        }

        auto destroyCImg = [&](CImg &img) {
            if (img.view)
                vkDestroyImageView(device, img.view, nullptr);
            if (img.image)
                vkDestroyImage(device, img.image, nullptr);
            if (img.memory)
                vkFreeMemory(device, img.memory, nullptr);
            img = {};
        };

        destroyCImg(workImg[0]);
        destroyCImg(workImg[1]);
        for (auto &img : histImg)
            destroyCImg(img);
        destroyCImg(outImg);

        if (computeSampler)
            vkDestroySampler(device, computeSampler, nullptr);
        if (compDSPool)
            vkDestroyDescriptorPool(device, compDSPool, nullptr);
        if (compPipeline)
            vkDestroyPipeline(device, compPipeline, nullptr);
        if (compPipeLayout)
            vkDestroyPipelineLayout(device, compPipeLayout, nullptr);
        if (compDSLayout)
            vkDestroyDescriptorSetLayout(device, compDSLayout, nullptr);
        if (stagingBuf)
            vkDestroyBuffer(device, stagingBuf, nullptr);
        if (stagingMem)
            vkFreeMemory(device, stagingMem, nullptr);
        if (readbackBuf)
            vkDestroyBuffer(device, readbackBuf, nullptr);
        if (readbackMem)
            vkFreeMemory(device, readbackMem, nullptr);
    }
};

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    try {
        ComputeWindow window(args.path, args.width, args.height, args.fullscreen, args.camera_index);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
