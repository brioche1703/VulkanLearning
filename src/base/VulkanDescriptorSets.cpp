#include "../../include/VulkanLearning/base/VulkanDescriptorSets.hpp"

namespace VulkanLearning {

    VulkanDescriptorSets::VulkanDescriptorSets(VulkanDevice* device, 
            VulkanSwapChain* swapChain, 
            VulkanDescriptorSetLayout* descriptorSetLayout,
            VulkanDescriptorPool* descriptorPool,
            std::vector<VulkanBuffer*> uniformBuffers,
            VkDeviceSize uniformBufferSize,
            VkImageView textureImageView,
            VkSampler textureSampler) :
    m_device(device), m_swapChain(swapChain), 
        m_descriptorSetLayout(descriptorSetLayout),
        m_descriptorPool(descriptorPool), m_uniformBuffers(uniformBuffers),
        m_uniformBufferSize(uniformBufferSize),
        m_textureImageView(textureImageView),
        m_textureSampler(textureSampler)
        {
            create();
        }

    VulkanDescriptorSets::~VulkanDescriptorSets() {}


    void VulkanDescriptorSets::create() {
        std::vector<VkDescriptorSetLayout> layouts(m_swapChain->getImages().size(), m_descriptorSetLayout->getDescriptorSetLayout());
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool->getDescriptorPool();
        allocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapChain->getImages().size());
        allocInfo.pSetLayouts = layouts.data();

        m_descriptorSets.resize(m_swapChain->getImages().size());

        if (vkAllocateDescriptorSets(m_device->getLogicalDevice(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Descriptor set allocation failed!");
        }

        for (size_t i = 0; i < m_swapChain->getImages().size(); i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_uniformBuffers[i]->getBuffer();
            bufferInfo.offset = 0;
            bufferInfo.range = m_uniformBufferSize;

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = m_textureImageView;
            imageInfo.sampler = m_textureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = m_descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m_descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(m_device->getLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
}
