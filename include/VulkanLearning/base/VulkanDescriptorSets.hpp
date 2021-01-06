#pragma once

#include <vulkan/vulkan.h>

#include <array>

#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanDescriptorSetLayout.hpp"
#include "VulkanDescriptorPool.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanTexture.hpp"

namespace VulkanLearning {
    class VulkanDescriptorSets {
        private:
            std::vector<VkDescriptorSet> m_descriptorSets;

            VulkanDevice m_device;
            VulkanDescriptorSetLayout m_descriptorSetLayout;
            VulkanDescriptorPool m_descriptorPool;

        public:
            VulkanDescriptorSets();
            VulkanDescriptorSets(VulkanDevice device, 
                    VulkanDescriptorSetLayout descriptorSetLayout,
                    VulkanDescriptorPool descriptorPool);

            ~VulkanDescriptorSets(); 

            inline std::vector<VkDescriptorSet> getDescriptorSets() { return m_descriptorSets; }
            inline std::vector<VkDescriptorSet>* getDescriptorSetsPointer() { return &m_descriptorSets; }

            void update(std::vector<VkWriteDescriptorSet> descriptorWrites, 
                    uint32_t imageIndex);

            void create(uint32_t descriptorSetCount);
    };
}
