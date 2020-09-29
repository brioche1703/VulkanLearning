#pragma once

#include <vulkan/vulkan.h>

#include <array>

#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"

namespace VulkanLearning {
    class VulkanDescriptorPool {
        private:
            VkDescriptorPool m_descriptorPool;

            VulkanDevice* m_device;
            VulkanSwapChain* m_swapChain;

        public:
            VulkanDescriptorPool(VulkanDevice* device, VulkanSwapChain* swapChain);
            ~VulkanDescriptorPool(); 

            inline VkDescriptorPool getDescriptorPool() { return m_descriptorPool; }

            void create();
    };
}
