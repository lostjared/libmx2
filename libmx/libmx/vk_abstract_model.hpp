#ifndef __ABSTRACT__
#define __ABSTRACT__

#include"vk.hpp"
#include"vk_model.hpp"

namespace mx {

    class VKAbstractModel {
    public:
        VKAbstractModel() = default;
        VKAbstractModel(const VKAbstractModel &) = delete;
        VKAbstractModel(VKAbstractModel&&) = delete;
        VKAbstractModel &operator=(const VKAbstractModel &) = delete;
        VKAbstractModel &operator=(VKAbstractModel &&) = delete;

        void load(VKWindow *win, const std::string &model, const std::string &text, const std::string &path, float scale);
        void setShaders(const std::string &vert, const std::string &frag);
        void render(int cmd);   
        void cleanup(VKWindow *win);
        void resize(VKWindow *win);
        void updateUBO(VKWindow *win, uint32_t imageIndex, const mx::UniformBufferObject &ubo);

        struct TexEntry {
            VkImage        image  = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkImageView    view   = VK_NULL_HANDLE;
            uint32_t       w = 0, h = 0;
        };
      
        mx::MXModel obj;
        std::vector<TexEntry> modelTextures;
        std::vector<VkDescriptorSet> modelDescriptorSets;
        glm::vec3 modelCenterOffset;
        float modelRenderScale;
    private:
        VkDescriptorPool modelDescriptorPool = VK_NULL_HANDLE;
        std::vector<VkBuffer>       modelUniformBuffers;
        std::vector<VkDeviceMemory> modelUniformBufferMemory;
        std::vector<void*>          modelUniformBuffersMapped;
        friend class VKWindow;
        void loadModelTextures(VKWindow *win, const std::string &tex, const std::string  &path);
        void createModelDescriptorPool(VKWindow *win);
        void createModelDescriptorSets(VKWindow *win);
    };
}


#endif