#pragma once

#include <vulkan/vulkan.h>

#include "VulkanSwapChain.hpp"

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


        public:

            VulkanImageResource(VulkanDevice* device, VulkanSwapChain* swapChain,
                    VkFormat format, VkImageUsageFlags usage, 
                    VkImageAspectFlags aspect);
            ~VulkanImageResource();

            inline VkImage getImage() { return m_image; }
            inline VkDeviceMemory getDeviceMemory() { return m_imageMemory; }
            inline VkImageView getImageView() { return m_imageView; }

            void create();

            void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, 
                    VkSampleCountFlagBits numSamples, VkFormat format, 
                    VkImageTiling tiling, VkImageUsageFlags usage, 
                    VkMemoryPropertyFlags properties);

            VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

    };

}
