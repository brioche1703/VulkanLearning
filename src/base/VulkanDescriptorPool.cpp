#include "VulkanDescriptorPool.hpp"

namespace VulkanLearning {

    VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice* device
            , VulkanSwapChain* swapChain) 
        : m_device(device), m_swapChain(swapChain) {
        }

    void VulkanDescriptorPool::create(const std::vector<VkDescriptorPoolSize> poolSizes) {
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(
                m_swapChain->getImages().size());
        poolInfo.flags = 0;

        if (vkCreateDescriptorPool(m_device->getLogicalDevice(), 
                    &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Descriptor pool creation failed!");
        }
    }

    void VulkanDescriptorPool::create(const std::vector<VkDescriptorPoolSize> poolSizes, const uint32_t maxSets) {
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = maxSets;
        poolInfo.flags = 0;

        if (vkCreateDescriptorPool(m_device->getLogicalDevice(), 
                    &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Descriptor pool creation failed!");
        }
    }
}
