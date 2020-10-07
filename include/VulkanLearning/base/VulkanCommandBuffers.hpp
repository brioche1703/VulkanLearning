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

            template<std::size_t SIZE>
            void create(const std::array<VkClearValue, SIZE>& clearValues) {
                m_commandBuffers.resize(m_swapChain->getFramebuffers().size());

                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.commandPool = m_commandPool->getCommandPool();
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocInfo.commandBufferCount = (uint32_t) m_commandBuffers.size();

                if (vkAllocateCommandBuffers(m_device->getLogicalDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
                    throw std::runtime_error("Command buffers allocation failed!");
                }

                for (size_t i = 0; i < m_commandBuffers.size(); i++) {
                    VkCommandBufferBeginInfo beginInfo{};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = 0;
                    beginInfo.pInheritanceInfo = nullptr;

                    if (vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                        throw std::runtime_error("Begin recording of a command buffer failed!");
                    }

                    VkRenderPassBeginInfo renderPassInfo{};
                    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassInfo.renderPass = m_renderPass->getRenderPass(); 
                    renderPassInfo.framebuffer = m_swapChain->getFramebuffers()[i];
                    renderPassInfo.renderArea.offset = {0, 0};
                    renderPassInfo.renderArea.extent = m_swapChain->getExtent();

                    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
                    renderPassInfo.pClearValues = clearValues.data();

                    vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

                    VkBuffer vertexBuffers[] = {m_vertexBuffer->getBuffer()};
                    VkDeviceSize offsets[] = {0};
                    vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);
                    vkCmdBindIndexBuffer(m_commandBuffers[i], m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

                    vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets->getDescriptorSets()[i], 0, nullptr);

                    vkCmdDrawIndexed(m_commandBuffers[i], m_indexCount, 1, 0, 0, 0);
                    vkCmdEndRenderPass(m_commandBuffers[i]);

                    if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS) {
                        throw std::runtime_error("Recording of a command buffer failed!");
                    }
                }
            }

            void cleanup();

    };
}
