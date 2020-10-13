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
    }

    VulkanCommandBuffers::~VulkanCommandBuffers() {}

    void VulkanCommandBuffers::cleanup() {
        vkFreeCommandBuffers(m_device->getLogicalDevice(), 
                m_commandPool->getCommandPool(), 
                static_cast<uint32_t>(
                    m_commandBuffers.size()), 
                m_commandBuffers.data());
    }

}
