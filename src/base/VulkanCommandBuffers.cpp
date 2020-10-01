#include "../../include/VulkanLearning/base/VulkanCommandBuffers.hpp"

namespace VulkanLearning {

    VulkanCommandBuffers::VulkanCommandBuffers(
            VulkanDevice* device, VulkanSwapChain* swapChain,
            VulkanCommandPool* commandPool, VulkanRenderPass* renderPass,
            VulkanBuffer* vertexBuffer, VulkanBuffer* indexBuffer,
            uint32_t indexCount, VkPipeline graphicsPipeline,
            VkPipelineLayout pipelineLayout, VulkanDescriptorSets* descriptorSets
            ) 
        : m_device(device), m_swapChain(swapChain), m_commandPool(commandPool),
        m_renderPass(renderPass), m_vertexBuffer(vertexBuffer), 
        m_indexBuffer(indexBuffer), m_indexCount(indexCount),
        m_graphicsPipeline(graphicsPipeline), m_pipelineLayout(pipelineLayout),
        m_descriptorSets(descriptorSets)
    {
        create();
    }
    VulkanCommandBuffers::~VulkanCommandBuffers() {}

    void VulkanCommandBuffers::create() {
        m_commandBuffers.resize(m_swapChain->getFramebuffers().size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool->getCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) m_commandBuffers.size();

        if (vkAllocateCommandBuffers(m_device->getLogicalDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Command buffers allocation failed!");
        }

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

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

    void VulkanCommandBuffers::cleanup() {
        vkFreeCommandBuffers(m_device->getLogicalDevice(), 
                m_commandPool->getCommandPool(), 
                static_cast<uint32_t>(
                    m_commandBuffers.size()), 
                m_commandBuffers.data());
    }
}
