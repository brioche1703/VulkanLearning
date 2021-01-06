#pragma once 

#include <vulkan/vulkan.h>

#include  "imgui.h"
#include  "glm/glm.hpp"

#include  "VulkanDevice.hpp"
#include  "VulkanBuffer.hpp"
#include  "VulkanDescriptorPool.hpp"
#include  "VulkanDescriptorSetLayout.hpp"
#include  "VulkanDescriptorSets.hpp"
#include  "VulkanRenderPass.hpp"
#include  "VulkanShaderModule.hpp"

namespace VulkanLearning {
    class UI {
        private:
            VulkanDevice m_device;
            VkQueue m_queue;

            VkSampleCountFlagBits m_rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            uint32_t m_subpass = 0;

            VulkanBuffer m_vertexBuffer;
            VulkanBuffer m_indexBuffer;

            int32_t m_vertexCount = 0;
            int32_t m_indexCount = 0;

            std::vector<VkPipelineShaderStageCreateInfo> m_shaders;

            VulkanDescriptorPool m_descriptorPool;
            VulkanDescriptorSetLayout m_descriptorSetLayout;
            VkDescriptorSet m_descriptorSet;
            VkPipelineLayout m_pipelineLayout;
            VkPipeline m_pipeline;

            VkDeviceMemory m_fontMemory = VK_NULL_HANDLE;
            VkImage m_fontImage = VK_NULL_HANDLE;
            VkImageView m_fontView = VK_NULL_HANDLE;
            VkSampler m_sampler;

            struct PushConstBlock {
                glm::vec2 scale;
                glm::vec2 translate;
            } pushConstBlock;

        public:
            bool visible = true;
            bool updated = false;
            float scale = 1.0f;

        public:

            UI();
            ~UI();

            void create(VulkanDevice device, VulkanRenderPass renderPass);

            void preparePipeline(const VkPipelineCache pipelineCache, const VulkanRenderPass renderPass);
            void prepareResources();

            bool update();
            void draw(const VulkanCommandBuffer commandBuffer);
            void resize(uint32_t width, uint32_t height);

            void  freeResources();

            bool header(const char* caption);
            bool checkBox(const char* caption, bool* value);
            bool checkBox(const char* caption, int32_t* value);
            bool inputFloat(const char* caption, float* value, float step, uint32_t precision);
            bool sliderFloat(const char* caption, float* value, float min, float max);
            bool sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max);
            bool comboBox(const char* caption, int32_t* itemIndex, std::vector<std::string> items);
            bool button(const char* caption);
            void text(const char *formatstr, ...);
    };
}
