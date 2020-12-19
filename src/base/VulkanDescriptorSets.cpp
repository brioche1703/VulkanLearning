#include "VulkanDescriptorSets.hpp"

namespace VulkanLearning {

    VulkanDescriptorSets::VulkanDescriptorSets() {}

    VulkanDescriptorSets::VulkanDescriptorSets(
            VulkanDevice device, 
            VulkanSwapChain swapChain, 
            VulkanDescriptorSetLayout descriptorSetLayout,
            VulkanDescriptorPool descriptorPool) 
        :
            m_device(device), 
            m_swapChain(swapChain), 
            m_descriptorSetLayout(descriptorSetLayout),
            m_descriptorPool(descriptorPool)
    {
    }

    VulkanDescriptorSets::~VulkanDescriptorSets() {}

    void VulkanDescriptorSets::create() {
        std::vector<VkDescriptorSetLayout> layouts(m_swapChain.getImages().size(), 
                m_descriptorSetLayout.getDescriptorSetLayout());

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool.getDescriptorPool();
        allocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapChain.getImages().size());
        allocInfo.pSetLayouts = layouts.data();

        m_descriptorSets.resize(m_swapChain.getImages().size());

        if (vkAllocateDescriptorSets(m_device.getLogicalDevice(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Descriptor set allocation failed!");
        }

    }

    void VulkanDescriptorSets::update(std::vector<VkWriteDescriptorSet> descriptorWrites, uint32_t imageIndex) {
        for (int j = 0; j < descriptorWrites.size(); j++) {
            descriptorWrites[j].dstSet =
                m_descriptorSets[imageIndex];
        }

        vkUpdateDescriptorSets(m_device.getLogicalDevice(), 
                static_cast<uint32_t>(descriptorWrites.size()), 
                descriptorWrites.data(), 0, nullptr);
    }
}
