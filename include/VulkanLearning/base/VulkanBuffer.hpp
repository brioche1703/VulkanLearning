#pragma once

#include <vulkan/vulkan.h>

#include "VulkanDevice.hpp"
#include "VulkanCommandPool.hpp"

namespace VulkanLearning {

    class VulkanBuffer {
        private:
            VkBuffer m_buffer;
            VkDeviceMemory m_bufferMemory;
            VkBufferUsageFlags m_usage;

            VulkanDevice* m_device;
            VulkanCommandPool* m_commandPool;

        public:
            VulkanBuffer(VulkanDevice* device, VulkanCommandPool* commandPool);
            ~VulkanBuffer();

            inline VkBuffer getBuffer() { return m_buffer; }
            inline VkDeviceMemory getBufferMemory() { return m_bufferMemory; }
            inline VkBuffer* getBufferPointer() { return &m_buffer; }
            inline VkDeviceMemory* getBufferMemoryPointer() { return &m_bufferMemory; }

            template<typename T>
            void createWithStagingBuffer(std::vector<T> data, VkBufferUsageFlags usage)
            {
                VkDeviceSize bufferSize = sizeof(data[0]) * data.size();
                VkBuffer stagingBuffer;
                VkDeviceMemory stagingBufferMemory;

                createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

                void* dataAux;
                vkMapMemory(m_device->getLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &dataAux);
                memcpy(dataAux, data.data(), (size_t) bufferSize);
                vkUnmapMemory(m_device->getLogicalDevice(), stagingBufferMemory);

                createBuffer(bufferSize, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_buffer, m_bufferMemory);

                copyBuffer(stagingBuffer, m_buffer, bufferSize);

                vkDestroyBuffer(m_device->getLogicalDevice(), stagingBuffer, nullptr);
                vkFreeMemory(m_device->getLogicalDevice(), stagingBufferMemory, nullptr);
            }

            void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

            void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

            VkCommandBuffer beginSingleTimeCommands();
            void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    };
}
