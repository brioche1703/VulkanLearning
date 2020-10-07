#include "../../include/VulkanLearning/base/VulkanDescriptorSetLayout.hpp"

#include <array>

namespace VulkanLearning {
            
    VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice* device) 
        : m_device(device){
    }
    
    void VulkanDescriptorSetLayout::create(std::vector<VkDescriptorSetLayoutBinding> bindings) {
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(m_device->getLogicalDevice(), 
                    &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Set layout descriptor creation failed!");
        }
    }

    void VulkanDescriptorSetLayout::cleanup() {
        vkDestroyDescriptorSetLayout(m_device->getLogicalDevice(), m_descriptorSetLayout, nullptr);
    }
}
