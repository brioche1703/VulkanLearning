#include <vulkan/vulkan_core.h>

#include <assert.h>
#include <string.h>

#include "VulkanBuffer.hpp"
#include "VulkanCommandBuffer.hpp"

namespace VulkanLearning {

    VulkanBuffer::VulkanBuffer() {}

    VulkanBuffer::VulkanBuffer(VulkanDevice device)
        : m_device(device) {}

    VulkanBuffer::~VulkanBuffer() {}
    
    void VulkanBuffer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
            VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_device.getLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Buffer creation failed!");
        }

        m_size = size;

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device.getLogicalDevice(), buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_device.findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_device.getLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("Memory allocation failed!");
        }

        vkBindBufferMemory(m_device.getLogicalDevice(), buffer, bufferMemory, 0);
        setupDescriptor();
    }

    void VulkanBuffer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
            VkMemoryPropertyFlags properties) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_device.getLogicalDevice(), &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS) {
            throw std::runtime_error("Buffer creation failed!");
        }

        m_size = size;

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device.getLogicalDevice(), m_buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_device.findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_device.getLogicalDevice(), &allocInfo, nullptr, &m_bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("Memory allocation failed!");
        }

        vkBindBufferMemory(m_device.getLogicalDevice(), m_buffer, m_bufferMemory, 0);
        setupDescriptor();
    }

    void VulkanBuffer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
            VkMemoryPropertyFlags properties, void* data) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_device.getLogicalDevice(), &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS) {
            throw std::runtime_error("Buffer creation failed!");
        }

        m_size = size;

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device.getLogicalDevice(), m_buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_device.findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_device.getLogicalDevice(), &allocInfo, nullptr, &m_bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("Memory allocation failed!");
        }

        if (data != nullptr)
        {
            vkMapMemory(m_device.getLogicalDevice(), m_bufferMemory, 0, size, 0, &m_mappedMemory);
            memcpy(m_mappedMemory, data, size);
            if ((properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                VkMappedMemoryRange mappedRange = {};
                mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                mappedRange.memory = m_bufferMemory;
                mappedRange.offset = 0;
                mappedRange.size = size;
                vkFlushMappedMemoryRanges(m_device.getLogicalDevice(), 1, &mappedRange);
            }
            vkUnmapMemory(m_device.getLogicalDevice(), m_bufferMemory);
        }

        vkBindBufferMemory(m_device.getLogicalDevice(), m_buffer, m_bufferMemory, 0);
        setupDescriptor();
    }

    void VulkanBuffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VulkanCommandBuffer commandBuffer;
        commandBuffer.create(&m_device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer.getCommandBuffer(), srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer.getCommandBuffer());
    }
    
    void VulkanBuffer::copyBufferToImage(VkImage image, uint32_t width, uint32_t height) {
        VulkanCommandBuffer commandBuffer;
        commandBuffer.create(&m_device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(commandBuffer.getCommandBuffer(), m_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        commandBuffer.flushCommandBuffer(&m_device, true);
    }

    void VulkanBuffer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_device.getGraphicsQueue());

        vkFreeCommandBuffers(m_device.getLogicalDevice(), m_device.getCommandPool(), 1, &commandBuffer);
    }

    VkResult VulkanBuffer::map(VkDeviceSize size, VkDeviceSize offset) {
            return vkMapMemory(
                    m_device.getLogicalDevice(), 
                    m_bufferMemory, 
                    offset, 
                    size, 
                    0, 
                    &m_mappedMemory);
    }

    void VulkanBuffer::unmap() {
        if (m_mappedMemory) {
            vkUnmapMemory(m_device.getLogicalDevice(), m_bufferMemory);
            m_mappedMemory = nullptr;
        }
    }

    VkResult VulkanBuffer::bind(VkDeviceSize offset) {
        return vkBindBufferMemory(m_device.getLogicalDevice(), 
               m_buffer, m_bufferMemory, offset);
    }

    void VulkanBuffer::copyTo(void* data, VkDeviceSize size) {
        assert(m_mappedMemory);
        memcpy(m_mappedMemory, data, size);
    }

    void VulkanBuffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset) {
        m_descriptor.offset = offset;
        m_descriptor.buffer = m_buffer;
        m_descriptor.range = size;
    }

    void VulkanBuffer::flush(VkDeviceSize size, VkDeviceSize offset) {
        VkMappedMemoryRange memoryRange = {};
        memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memoryRange.memory = m_bufferMemory;
        memoryRange.offset = offset;
        memoryRange.size = size;
    }

    void VulkanBuffer::cleanup() {
        vkFreeMemory(m_device.getLogicalDevice(), m_bufferMemory, nullptr);
        vkDestroyBuffer(m_device.getLogicalDevice(), m_buffer, nullptr);
    }

}
