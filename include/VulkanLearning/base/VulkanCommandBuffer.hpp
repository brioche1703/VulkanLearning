#pragma once

#include <vulkan/vulkan.h>

#include "VulkanCommandPool.hpp"
#include "VulkanDevice.hpp"

namespace VulkanLearning {

    class VulkanCommandBuffer {
        private:
            VkCommandBuffer m_commandBuffer;

        public:
            VulkanCommandBuffer();
            ~VulkanCommandBuffer();

            inline VkCommandBuffer getCommandBuffer() { return m_commandBuffer; }
            inline VkCommandBuffer* getCommandBufferPointer() { return &m_commandBuffer; }

            void beginSingleTimeCommands(VulkanDevice* device, VulkanCommandPool* commandPool);
            void endSingleTimeCommands(VulkanDevice* device, VulkanCommandPool* commandPool);
    };
}
