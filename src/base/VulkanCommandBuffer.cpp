#include "../../include/VulkanLearning/base/VulkanCommandBuffer.hpp"

namespace VulkanLearning {

    VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice* device, 
            VulkanCommandPool* commandPool)
        : m_device(device), m_commandPool(commandPool) {
        }

    VulkanCommandBuffer::~VulkanCommandBuffer() {}

    void VulkanCommandBuffer::beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};

        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_commandPool->getCommandPool();
        allocInfo.commandBufferCount = 1;

        vkAllocateCommandBuffers(m_device->getLogicalDevice(), &allocInfo, &m_commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
    }

    void VulkanCommandBuffer::endSingleTimeCommands() {
        vkEndCommandBuffer(m_commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_commandBuffer;

        vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_device->getGraphicsQueue());

        vkFreeCommandBuffers(m_device->getLogicalDevice(), m_commandPool->getCommandPool(), 1, &m_commandBuffer);
    }
}
