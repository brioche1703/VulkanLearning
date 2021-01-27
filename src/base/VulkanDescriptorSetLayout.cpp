#include "VulkanDescriptorSetLayout.hpp"
#include "VulkanTools.hpp"

#include <array>

namespace VulkanLearning {
    VulkanDescriptorSetLayout::VulkanDescriptorSetLayout() {}

    VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice device) 
        : m_device(device){
    }

    VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {}
    
    void VulkanDescriptorSetLayout::create(std::vector<VkDescriptorSetLayoutBinding> bindings) {
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(
                    m_device.getLogicalDevice(), 
                    &layoutInfo, 
                    nullptr, 
                    &m_descriptorSetLayout));
    }

    void VulkanDescriptorSetLayout::cleanup() {
        vkDestroyDescriptorSetLayout(
                m_device.getLogicalDevice(),
                m_descriptorSetLayout, 
                nullptr);
    }
}
