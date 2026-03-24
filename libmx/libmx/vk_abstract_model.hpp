/**
 * @file vk_abstract_model.hpp
 * @brief High-level Vulkan 3-D model wrapper with texture and descriptor management.
 *
 * VKAbstractModel wraps MXModel, automatically handling texture loading,
 * uniform buffer allocation, descriptor pool/set creation, and bounding-box
 * centring so that callers only need to supply file paths and a scale.
 */
#ifndef __ABSTRACT__
#define __ABSTRACT__

#include "vk.hpp"
#include "vk_model.hpp"

namespace mx {

    /**
     * @class VKAbstractModel
     * @brief Convenience wrapper that owns an MXModel plus its Vulkan textures,
     *        UBOs, and descriptor sets.
     *
     * Typical usage:
     * @code
     *   VKAbstractModel mdl;
     *   mdl.load(win, "mesh.obj", "textures.tex", "assets", 1.0f);
     *   // each frame:
     *   mdl.updateUBO(win, imageIndex, ubo);
     *   // on resize:
     *   mdl.resize(win);
     *   // on shutdown:
     *   mdl.cleanup(win);
     * @endcode
     */
    class VKAbstractModel {
      public:
        /** @brief Default constructor. */
        VKAbstractModel() = default;
        VKAbstractModel(const VKAbstractModel &) = delete;
        VKAbstractModel(VKAbstractModel &&) = delete;
        VKAbstractModel &operator=(const VKAbstractModel &) = delete;
        VKAbstractModel &operator=(VKAbstractModel &&) = delete;

        /**
         * @brief Load a model, its textures, and create all GPU resources.
         * @param win    Owning Vulkan window (provides device handles).
         * @param model  Path to the mesh file (.obj / .mxmod / .mxmod.z).
         * @param text   Path to a .tex file listing texture image filenames.
         * @param path   Directory prefix prepended to each texture filename.
         * @param scale  Uniform scale applied to all vertex positions.
         */
        void load(VKWindow *win, const std::string &model, const std::string &text, const std::string &path, float scale);

        /**
         * @brief Set custom vertex and fragment shaders and create a per-model pipeline.
         * @param win  Owning Vulkan window.
         * @param vert Path to the vertex shader SPIR-V file.
         * @param frag Path to the fragment shader SPIR-V file.
         */
        void setShaders(VKWindow *win, const std::string &vert, const std::string &frag);

        /**
         * @brief Record rendering commands (currently a placeholder).
         * @param cmd Command buffer index.
         */
        void render(int cmd);

        /**
         * @brief Destroy all Vulkan resources owned by this model.
         * @param win Owning Vulkan window.
         */
        void cleanup(VKWindow *win);

        /**
         * @brief Recreate UBOs and descriptor sets after a swapchain resize.
         * @param win Owning Vulkan window.
         */
        void resize(VKWindow *win);

        /**
         * @brief Copy a UBO into the mapped buffer for a given frame.
         * @param win        Owning Vulkan window.
         * @param imageIndex Swapchain image index for this frame.
         * @param ubo        Uniform data to upload.
         */
        void updateUBO(VKWindow *win, uint32_t imageIndex, const mx::UniformBufferObject &ubo);

        /**
         * @struct TexEntry
         * @brief Vulkan image, memory, and view for one loaded texture.
         */
        struct TexEntry {
            VkImage image = VK_NULL_HANDLE;         ///< Vulkan image handle.
            VkDeviceMemory memory = VK_NULL_HANDLE; ///< Device memory backing the image.
            VkImageView view = VK_NULL_HANDLE;      ///< Image view for shader sampling.
            uint32_t w = 0, h = 0;                  ///< Texture dimensions in pixels.
        };

        mx::MXModel obj;                                  ///< Underlying mesh data and GPU buffers.
        std::vector<TexEntry> modelTextures;              ///< Per-sub-mesh texture entries.
        std::vector<VkDescriptorSet> modelDescriptorSets; ///< Descriptor sets (frames x textures).
        glm::vec3 modelCenterOffset;                      ///< Translation to centre the bounding box at origin.
        float modelRenderScale;                           ///< Scale factor normalising model extent.

        /**
         * @brief Get the per-model pipeline, or VK_NULL_HANDLE if using the window default.
         * @return Pipeline handle (fill mode).
         */
        VkPipeline getModelPipeline() const { return modelPipeline; }

        /**
         * @brief Get the per-model wireframe pipeline, or VK_NULL_HANDLE if not available.
         * @return Pipeline handle (wireframe mode).
         */
        VkPipeline getModelPipelineWireframe() const { return modelPipelineWireframe; }

        /**
         * @brief Create a lightweight instance sharing geometry & textures from a source model.
         *
         * Only creates its own UBOs and descriptor sets. The source model must
         * outlive this instance and be cleaned up after all instances.
         *
         * @param win    Owning Vulkan window.
         * @param source Fully-loaded model to share geometry/textures from.
         */
        void loadInstance(VKWindow *win, VKAbstractModel &source);

        /**
         * @brief Check whether this model has custom shaders set.
         * @return @c true if setShaders() was called.
         */
        bool hasCustomShaders() const { return !vertShaderPath_.empty(); }

        /** @brief Check whether this is a lightweight instance (shared geometry). */
        bool isInstance() const { return isInstance_; }

      private:
        bool isInstance_ = false;                              ///< True if geometry/textures are shared from another model.
        VkDescriptorPool modelDescriptorPool = VK_NULL_HANDLE; ///< Descriptor pool for this model.
        VkPipeline modelPipeline = VK_NULL_HANDLE;             ///< Per-model graphics pipeline (fill).
        VkPipeline modelPipelineWireframe = VK_NULL_HANDLE;    ///< Per-model wireframe pipeline.
        std::string vertShaderPath_;                           ///< Vertex shader SPIR-V path.
        std::string fragShaderPath_;                           ///< Fragment shader SPIR-V path.
        std::vector<VkBuffer> modelUniformBuffers;             ///< Per-frame uniform buffers.
        std::vector<VkDeviceMemory> modelUniformBufferMemory;  ///< Memory for per-frame UBOs.
        std::vector<void *> modelUniformBuffersMapped;         ///< Persistently mapped UBO pointers.
        friend class VKWindow;

        /** @brief Clean up only UBO and descriptor resources (used by instances). */
        void cleanupInstanceResources(VKWindow *win);

        /**
         * @brief Load texture images listed in a .tex file and upload to GPU.
         * @param win  Owning Vulkan window.
         * @param tex  Path to the .tex manifest file.
         * @param path Directory prefix for image files.
         */
        void loadModelTextures(VKWindow *win, const std::string &tex, const std::string &path);

        /**
         * @brief Load textures using map_Kd paths from parsed MTL materials.
         * @param win  Owning Vulkan window.
         * @param path Directory prefix for image files.
         */
        void loadModelTexturesFromMTL(VKWindow *win, const std::string &path);

        /**
         * @brief Create the Vulkan descriptor pool sized for this model's textures.
         * @param win Owning Vulkan window.
         */
        void createModelDescriptorPool(VKWindow *win);

        /**
         * @brief Allocate and write descriptor sets binding UBOs and textures.
         * @param win Owning Vulkan window.
         */
        void createModelDescriptorSets(VKWindow *win);

        /**
         * @brief Create a per-model graphics pipeline from stored shader paths.
         * @param win Owning Vulkan window.
         */
        void createModelPipeline(VKWindow *win);
    };
} // namespace mx

#endif