#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <algorithm>
#include <memory>
#include <cmath>

#include "../../external/stb/stb_image.h"

#include "VulkanDevice.hpp"
#include "VulkanCommandPool.hpp"
#include "VulkanImageResource.hpp"

namespace VulkanLearning {

    class VulkanTexture {
        private:
            std::string m_filename;
            VulkanImageResource* m_textureImageResources;

            VkSampler m_sampler;

            int m_width, m_height;
            int m_channelCount;
            uint32_t m_mipLevels;

            VulkanDevice* m_device;
            VulkanCommandPool* m_commandPool;
            VulkanSwapChain* m_swapChain;

        public:
            VulkanTexture(std::string filename, VulkanDevice* device,
                    VulkanSwapChain* swapChain, 
                    VulkanCommandPool* commandPool);
            ~VulkanTexture();

            inline VkImage getImage() { return m_textureImageResources->getImage(); }
            inline VkDeviceMemory getDeviceMemory() { return m_textureImageResources->getDeviceMemory(); }
            inline VkImageView getImageView() { return m_textureImageResources->getImageView(); }
            inline uint32_t getMipLevels() { return m_mipLevels; }
            inline VkSampler getSampler() { return m_sampler; }

            void create();
            void generateMipmaps(VkFormat imageFormat, 
                    int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
            void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
            void createImageView(VkFormat format, 
                    VkImageAspectFlags aspectFlags);
            void createSampler();
    };
}
