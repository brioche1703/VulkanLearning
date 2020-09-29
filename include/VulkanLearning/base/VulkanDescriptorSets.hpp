#pragma once

#include <vulkan/vulkan.h>

#include <array>

#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanDescriptorSetLayout.hpp"
#include "VulkanDescriptorPool.hpp"
#include "VulkanBuffer.hpp"

namespace VulkanLearning {
    class VulkanDescriptorSets {
        private:
            std::vector<VkDescriptorSet> m_descriptorSets;

            VulkanDevice* m_device;
            VulkanSwapChain* m_swapChain;
            VulkanDescriptorSetLayout* m_descriptorSetLayout;
            VulkanDescriptorPool* m_descriptorPool;

            std::vector<VulkanBuffer*> m_uniformBuffers;
            VkDeviceSize m_uniformBufferSize;

            VkImageView m_textureImageView;
            VkSampler m_textureSampler;

        public:
            VulkanDescriptorSets(VulkanDevice* device, 
                    VulkanSwapChain* swapChain, 
                    VulkanDescriptorSetLayout* descriptorSetLayout,
                    VulkanDescriptorPool* descriptorPool,
                    std::vector<VulkanBuffer*> uniformBuffers,
                    VkDeviceSize uniformBufferSize,
                    VkImageView textureImageView,
                    VkSampler textureSampler);

            ~VulkanDescriptorSets(); 

            inline std::vector<VkDescriptorSet> getDescriptorSets() { return m_descriptorSets; }

            void create();
    };
}