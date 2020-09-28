#include "../../include/VulkanLearning/base/VulkanDescriptorSetLayout.hpp"

#include <array>

namespace VulkanLearning {
            
    VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice* device) {
        create(device);
    }
    
    void VulkanDescriptorSetLayout::create(VulkanDevice* device) {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerLayoutBinding.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device->getLogicalDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Set layout descriptor creation failed!");
        }
    }
}
