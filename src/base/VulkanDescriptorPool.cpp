#include "../../include/VulkanLearning/base/VulkanDescriptorPool.hpp"

namespace VulkanLearning {

    VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice* device
            , VulkanSwapChain* swapChain) 
        : m_device(device), m_swapChain(swapChain) {
            create();
        }

    void VulkanDescriptorPool::create() {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};

        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(
                m_swapChain->getImages().size());
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(
                m_swapChain->getImages().size());


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
}
