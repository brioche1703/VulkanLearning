#pragma once

#include <vulkan/vulkan.h>

#include "VulkanCommandPool.hpp"
#include "VulkanDevice.hpp"

namespace VulkanLearning {

    class VulkanCommandBuffer {
        private:
            VkCommandBuffer m_commandBuffer;

            VulkanDevice* m_device;
            VulkanCommandPool* m_commandPool;
        public:
            VulkanCommandBuffer(VulkanDevice* device, 
                    VulkanCommandPool* commandPool);
            ~VulkanCommandBuffer();

            inline VkCommandBuffer getCommandBuffer() { return m_commandBuffer; }
            inline VkCommandBuffer* getCommandBufferPointer() { return &m_commandBuffer; }

            void beginSingleTimeCommands();
            void endSingleTimeCommands();
    };
}
