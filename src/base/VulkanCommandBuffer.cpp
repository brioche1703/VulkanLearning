#include "VulkanCommandBuffer.hpp"
#include <vulkan/vulkan_core.h>

namespace VulkanLearning {

    VulkanCommandBuffer::VulkanCommandBuffer() {}

    VulkanCommandBuffer::~VulkanCommandBuffer() {}

    void VulkanCommandBuffer::create(VulkanDevice* device, VkCommandBufferLevel level, bool begin) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = level;
        allocInfo.commandPool = device->getCommandPool();
        allocInfo.commandBufferCount = 1;

        vkAllocateCommandBuffers(device->getLogicalDevice(), &allocInfo, &m_commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
    }

    void VulkanCommandBuffer::flushCommandBuffer(VulkanDevice* device, bool free) {
        if (m_commandBuffer == VK_NULL_HANDLE) {
            return;
        } 

        if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Command buffer end failed!");
        }

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_commandBuffer;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VkFence fence;
        if (vkCreateFence(device->getLogicalDevice(), &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
            throw std::runtime_error("Fence creation failed!");
        }

        if (vkQueueSubmit(device->getGraphicsQueue(), 1, &submitInfo, fence) != VK_SUCCESS) {
            throw std::runtime_error("Fence submition to queue failed!");
        }

        if (vkWaitForFences(device->getLogicalDevice(), 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
            throw std::runtime_error("Waiting for fence failed!");
        }

        vkDestroyFence(device->getLogicalDevice(), fence, nullptr);

        if (free)
        {
            vkFreeCommandBuffers(device->getLogicalDevice(), device->getCommandPool(), 1, &m_commandBuffer);
        }
    }
}
