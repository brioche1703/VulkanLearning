#pragma once

#include <vulkan/vulkan.h>

#include "VulkanSwapChain.hpp"
#include "VulkanCommandPool.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanBuffer.hpp"

namespace VulkanLearning {

    class VulkanImageResource {
        private:
            VkImage m_image;
            VkDeviceMemory m_imageMemory;
            VkImageView m_imageView;

            VkFormat m_format;
            VkImageUsageFlags m_usage;
            VkImageAspectFlags m_aspect;

            VulkanDevice* m_device;
            VulkanSwapChain* m_swapChain;
            VulkanCommandPool* m_commandPool;

        public:

            VulkanImageResource(VulkanDevice* device, VulkanSwapChain* swapChain,
                    VulkanCommandPool* commandPool, VkFormat format, 
                    VkImageUsageFlags usage, VkImageAspectFlags aspect);
            ~VulkanImageResource();

            inline VkImage getImage() { return m_image; }
            inline VkDeviceMemory getDeviceMemory() { return m_imageMemory; }
            inline VkImageView getImageView() { return m_imageView; }

            void create();

            void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, 
                    VkSampleCountFlagBits numSamples, VkFormat format, 
                    VkImageTiling tiling, VkImageUsageFlags usage, 
                    VkMemoryPropertyFlags properties);

            VkImageView createImageView(VkImage image, VkFormat format,
                    VkImageAspectFlags aspectFlags, uint32_t mipLevels);
            void createImageView(VkFormat format, 
                    VkImageAspectFlags aspectFlags, uint32_t mipLevels);

            void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, 
                    VkImageLayout newLayout, uint32_t mipLevels);

            void copyBufferToImage(VulkanBuffer buffer, uint32_t width, uint32_t height) {
                VulkanCommandBuffer commandBuffer(m_device, m_commandPool);
                commandBuffer.beginSingleTimeCommands();

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

                vkCmdCopyBufferToImage(commandBuffer.getCommandBuffer(), buffer.getBuffer(), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

                commandBuffer.endSingleTimeCommands();
            }
    };

}
