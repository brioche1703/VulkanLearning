#pragma once

#include <vulkan/vulkan.h>

#include "VulkanDevice.hpp"

namespace VulkanLearning {

    class VulkanCommandPool {
        private:
            VkCommandPool m_commandPool;
        public:
            VulkanCommandPool(VulkanDevice* device);
            ~VulkanCommandPool();

            inline VkCommandPool getCommandPool() { return m_commandPool; }

            void create(VulkanDevice* device);
    };
}
