#include "VulkanCommandPool.hpp"

namespace VulkanLearning {
    VulkanCommandPool::VulkanCommandPool(VulkanDevice* device) 
        : m_device(device) {
        create();
    }

    void VulkanCommandPool::create() {
        QueueFamilyIndices queueFamilyIndicies = m_device->getQueueFamilyIndices();

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndicies.graphicsFamily.value();

        if (vkCreateCommandPool(m_device->getLogicalDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
            throw std::runtime_error("One command pool creation failed!");
        }
    }

    void VulkanCommandPool::cleanup() {
        vkDestroyCommandPool(m_device->getLogicalDevice(), m_commandPool, nullptr);
    }
}
