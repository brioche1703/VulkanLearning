#pragma once

#include <vulkan/vulkan.h>

#include "VulkanDevice.hpp"
#include "VulkanCommandPool.hpp"

namespace VulkanLearning {

    class VulkanBuffer {
        private:
            VkBuffer m_buffer = VK_NULL_HANDLE;
            VkDeviceMemory m_bufferMemory = VK_NULL_HANDLE;
            VkBufferUsageFlags m_usage;
            VkDeviceSize m_size;
            void* m_mappedMemory = nullptr;

            VulkanDevice* m_device;
            VulkanCommandPool* m_commandPool;

        public:
            VulkanBuffer(VulkanDevice* device, VulkanCommandPool* commandPool);
            ~VulkanBuffer();

            inline VkBuffer getBuffer() { return m_buffer; }
            inline VkDeviceMemory getBufferMemory() { return m_bufferMemory; }
            inline VkBuffer* getBufferPointer() { return &m_buffer; }
            inline VkDeviceMemory* getBufferMemoryPointer() { return &m_bufferMemory; }
            inline VkDeviceSize getSize() { return m_size; }
            inline void* getMappedMemory() { return m_mappedMemory; }
            inline void** getMappedMemoryPointer() { return &m_mappedMemory; }

            /* Copy byffer into GPU accessible only by GPU */
            template<typename T>
            void createWithStagingBuffer(std::vector<T> data, VkBufferUsageFlags usage)
            {
                VkDeviceSize bufferSize = sizeof(data[0]) * data.size();
                VkBuffer stagingBuffer;
                VkDeviceMemory stagingBufferMemory;

                createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                        stagingBuffer, stagingBufferMemory);

                void* dataAux;
                vkMapMemory(m_device->getLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &dataAux);
                memcpy(dataAux, data.data(), (size_t) bufferSize);
                vkUnmapMemory(m_device->getLogicalDevice(), stagingBufferMemory);

                createBuffer(bufferSize, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                        m_buffer, m_bufferMemory);

                copyBuffer(stagingBuffer, m_buffer, bufferSize);

                vkDestroyBuffer(m_device->getLogicalDevice(), stagingBuffer, nullptr);
                vkFreeMemory(m_device->getLogicalDevice(), stagingBufferMemory, nullptr);
            }

            void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                    VkMemoryPropertyFlags properties, VkBuffer& buffer, 
                    VkDeviceMemory& bufferMemory);
            void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                    VkMemoryPropertyFlags properties);
            void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
            void copyBufferToImage(VkImage image, uint32_t width, uint32_t height);

            VkCommandBuffer beginSingleTimeCommands();
            void endSingleTimeCommands(VkCommandBuffer commandBuffer);

            VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
            void unmap();
            VkResult bind(VkDeviceSize offset = 0);
            void copyTo(void* data, VkDeviceSize size);

            void cleanup();
    };
}
