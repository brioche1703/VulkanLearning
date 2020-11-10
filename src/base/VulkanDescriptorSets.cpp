#include "../../include/VulkanLearning/base/VulkanDescriptorSets.hpp"

namespace VulkanLearning {

    VulkanDescriptorSets::VulkanDescriptorSets(
            VulkanDevice* device, 
            VulkanSwapChain* swapChain, 
            VulkanDescriptorSetLayout* descriptorSetLayout,
            VulkanDescriptorPool* descriptorPool,
            std::vector<std::vector<VulkanBuffer*>> uniformBuffers,
            std::vector<VkDeviceSize> uniformBufferSize,
            VulkanTexture* texture) :
    m_device(device), m_swapChain(swapChain), 
        m_descriptorSetLayout(descriptorSetLayout),
        m_descriptorPool(descriptorPool), m_uniformBuffers(uniformBuffers),
        m_uniformBufferSize(uniformBufferSize),
        m_texture(texture)
        {
        }

    VulkanDescriptorSets::~VulkanDescriptorSets() {}


    void VulkanDescriptorSets::create(
            VkDescriptorBufferInfo bufferInfo,
            std::vector<VkWriteDescriptorSet> descriptorWrites,
            VkDescriptorImageInfo *imageInfo) {
        std::vector<VkDescriptorSetLayout> layouts(m_swapChain->getImages().size(), 
                m_descriptorSetLayout->getDescriptorSetLayout());

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
            std::vector<VkWriteDescriptorSet> descriptorWritesAux = descriptorWrites;

            VkDescriptorBufferInfo bufferInfoAux = bufferInfo;
            bufferInfoAux.buffer = m_uniformBuffers[0][i]->getBuffer();
            bufferInfoAux.range = m_uniformBufferSize[0];

            descriptorWritesAux[0].pBufferInfo = &bufferInfoAux;
            descriptorWritesAux[2].pBufferInfo = &bufferInfoAux;

            if (imageInfo != nullptr) {
                VkDescriptorImageInfo imageInfoAux = *imageInfo;
                descriptorWritesAux[1].pImageInfo = &imageInfoAux;
            }

            for (int j = 0; j < descriptorWritesAux.size(); j++) {
                descriptorWritesAux[j].dstSet = m_descriptorSets[i];
            }

            vkUpdateDescriptorSets(m_device->getLogicalDevice(), 
                    static_cast<uint32_t>(descriptorWritesAux.size()), 
                    descriptorWritesAux.data(), 0, nullptr);
        }
    }

    void VulkanDescriptorSets::create(
            std::vector<VkWriteDescriptorSet> descriptorWrites) {

        std::vector<VkDescriptorSetLayout> layouts(m_swapChain->getImages().size(), 
                m_descriptorSetLayout->getDescriptorSetLayout());

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
            std::vector<VkWriteDescriptorSet> descriptorWritesAux = descriptorWrites;
            std::vector<VkDescriptorBufferInfo> bufferInfoAux = 
                std::vector<VkDescriptorBufferInfo>(descriptorWrites.size());

            for (size_t j = 0; j < descriptorWritesAux.size(); j++) {
                bufferInfoAux[j].offset = 0;
                bufferInfoAux[j].buffer = m_uniformBuffers[j][i]->getBuffer();
                bufferInfoAux[j].range = m_uniformBufferSize[j];

                descriptorWritesAux[j].pBufferInfo = &bufferInfoAux[j];
                descriptorWritesAux[j].dstSet = m_descriptorSets[i];
            }

            vkUpdateDescriptorSets(m_device->getLogicalDevice(), 
                    static_cast<uint32_t>(descriptorWritesAux.size()), 
                    descriptorWritesAux.data(), 0, nullptr);
        }
    }
}
