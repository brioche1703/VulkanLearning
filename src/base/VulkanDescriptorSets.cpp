#include "VulkanDescriptorSets.hpp"

namespace VulkanLearning {

    VulkanDescriptorSets::VulkanDescriptorSets() {}

    VulkanDescriptorSets::VulkanDescriptorSets(
            VulkanDevice device, 
            VulkanDescriptorSetLayout descriptorSetLayout,
            VulkanDescriptorPool descriptorPool) 
        :
            m_device(device), 
            m_descriptorSetLayout(descriptorSetLayout),
            m_descriptorPool(descriptorPool)
    {
    }

    VulkanDescriptorSets::~VulkanDescriptorSets() {}

    void VulkanDescriptorSets::create(uint32_t descriptorSetCount) {
        std::vector<VkDescriptorSetLayout> layouts(
                descriptorSetCount,
                m_descriptorSetLayout.getDescriptorSetLayout());

        m_descriptorSets.resize(descriptorSetCount);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool.getDescriptorPool();
        allocInfo.descriptorSetCount = descriptorSetCount;
        allocInfo.pSetLayouts = layouts.data();

        VK_CHECK_RESULT(vkAllocateDescriptorSets(
                    m_device.getLogicalDevice(), 
                    &allocInfo, 
                    m_descriptorSets.data()));

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
