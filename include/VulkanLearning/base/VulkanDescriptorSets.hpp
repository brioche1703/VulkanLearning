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

            VulkanDevice* m_device;
            VulkanSwapChain* m_swapChain;
            VulkanDescriptorSetLayout* m_descriptorSetLayout;
            VulkanDescriptorPool* m_descriptorPool;

            std::vector<std::vector<VulkanBuffer*>> m_uniformBuffers;

            VulkanTexture* m_texture;

        public:
            VulkanDescriptorSets(VulkanDevice* device, 
                    VulkanSwapChain* swapChain, 
                    VulkanDescriptorSetLayout* descriptorSetLayout,
                    VulkanDescriptorPool* descriptorPool);

            ~VulkanDescriptorSets(); 

            inline std::vector<VkDescriptorSet> getDescriptorSets() { return m_descriptorSets; }
            inline std::vector<VkDescriptorSet>* getDescriptorSetsPointer() { return &m_descriptorSets; }

            void update(std::vector<VkWriteDescriptorSet> descriptorWrites, 
                    uint32_t imageIndex);

            void create();
    };
}
