#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <vulkan/vulkan_core.h>

#include "VulkanDevice.hpp"
#include "VulkanShaderModule.hpp"
#include "VulkanRenderPass.hpp"
#include "VulkanDescriptorSetLayout.hpp"

namespace VulkanLearning {

    class VulkanGraphicsPipeline {
        private:
            VkPipeline m_graphicsPipeline;
            VkPipelineLayout m_pipelineLayout;

            VulkanDevice* m_device;
            VulkanSwapChain* m_swapChain;
            VulkanRenderPass* m_renderPass;
            VulkanDescriptorSetLayout* m_descriptorSetLayout;

        public:
            VulkanGraphicsPipeline(VulkanDevice* device, 
                    VulkanSwapChain* swapChain, VulkanRenderPass* renderPass,
                    VulkanDescriptorSetLayout* descriptorSetLayout);

            ~VulkanGraphicsPipeline();

            inline VkPipeline getGraphicsPipeline() { return m_graphicsPipeline; }
            inline VkPipeline* getGraphicsPipelinePointer() { return &m_graphicsPipeline; }
            inline VkPipelineLayout getPipelineLayout() { return m_pipelineLayout; }

            void create(
                    VulkanShaderModule vertShaderModule,
                    VulkanShaderModule fragShaderModule,
                    VkPipelineVertexInputStateCreateInfo vertexInputInfo,
                    VkPipelineLayoutCreateInfo pipelineLayoutInfo,
                    VkPipelineDepthStencilStateCreateInfo *depthStencil = nullptr);
    };

}
