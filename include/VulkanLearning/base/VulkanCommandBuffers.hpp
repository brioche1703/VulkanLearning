#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <vulkan/vulkan_core.h>

#include "VulkanDevice.hpp"
#include "VulkanCommandPool.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanRenderPass.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanDescriptorSets.hpp"

namespace VulkanLearning {
    class VulkanCommandBuffers {
        private:
            std::vector<VkCommandBuffer> m_commandBuffers;

            VulkanDevice* m_device;
            VulkanSwapChain* m_swapChain;
            VulkanCommandPool* m_commandPool;
            VulkanRenderPass* m_renderPass;
            VulkanBuffer* m_vertexBuffer;
            VulkanBuffer* m_indexBuffer;
            uint32_t m_indexCount;

            VkPipeline m_graphicsPipeline;
            VkPipelineLayout m_pipelineLayout;

            VulkanDescriptorSets* m_descriptorSets;

        public:
            VulkanCommandBuffers(
                    VulkanDevice* device,
                    VulkanSwapChain* swapChain,
                    VulkanCommandPool* commandPool,
                    VulkanRenderPass* renderPass,
                    VulkanBuffer* vertexBuffer,
                    VulkanBuffer* indexBuffer,
                    uint32_t indexCount,
                    VkPipeline graphicsPipeline,
                    VkPipelineLayout pipelineLayout,
                    VulkanDescriptorSets* descriptorSets
                    );

            ~VulkanCommandBuffers();

            inline std::vector<VkCommandBuffer> getCommandBuffers() { return m_commandBuffers; }
            inline std::vector<VkCommandBuffer>* getCommandBuffersPointer() { return &m_commandBuffers; }
            inline VkCommandBuffer* getCommandBufferPointer(uint32_t index) { return &m_commandBuffers[index]; }

            void create();


    };
}
