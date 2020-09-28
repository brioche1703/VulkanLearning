#include "../../include/VulkanLearning/base/VulkanCommandPool.hpp"

namespace VulkanLearning {
    VulkanCommandPool::VulkanCommandPool(VulkanDevice* device){
        create(device);
    }

    void VulkanCommandPool::create(VulkanDevice* device) {
        QueueFamilyIndices queueFamilyIndicies = device->getQueueFamilyIndices();

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndicies.graphicsFamily.value();
        poolInfo.flags = 0; 

        if (vkCreateCommandPool(device->getLogicalDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
            throw std::runtime_error("One command pool creation failed!");
        }
    }
}
