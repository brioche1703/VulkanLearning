#pragma once

#include <vulkan/vulkan.h>

#include "VulkanDevice.hpp"

namespace VulkanLearning {

    class VulkanCommandBuffer {
        private:
            VkCommandBuffer m_commandBuffer;

        public:
            VulkanCommandBuffer();
            ~VulkanCommandBuffer();

            inline VkCommandBuffer getCommandBuffer() const { return m_commandBuffer; }
            inline VkCommandBuffer* getCommandBufferPointer() { return &m_commandBuffer; }

            void create(VulkanDevice* device, VkCommandBufferLevel level, bool begin);

            void flushCommandBuffer(VulkanDevice* device, bool free);
            void flushCommandBuffer(VulkanDevice* device, VkQueue queue, bool free);
    };
}
