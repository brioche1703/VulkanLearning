#pragma once

#include <string>
#include <algorithm>
#include <memory>
#include <cmath>

#include "ktx.h"
#include "stb_image.h"

#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"

namespace VulkanLearning {

    class VulkanTexture {
        protected:
            VulkanDevice* m_device;
            VkImage m_image;
            VkImageLayout m_imageLayout;
            VkDeviceMemory m_deviceMemory;
            VkImageView m_view;
            int m_width;
            int m_height;
            uint32_t m_mipLevels;
            uint32_t m_layourCount;
            int m_channelCount;
            VkDescriptorImageInfo m_descriptor;
            VkSampler m_sampler;

        public:
            VulkanTexture() {}
            ~VulkanTexture() {}

            inline VkImageView getView() { return m_view; }
            inline VkSampler getSampler() { return m_sampler; }
            inline VkImageLayout getImageLayout() { return m_imageLayout; }

            ktxResult loadKTXFile(std::string filename, ktxTexture **target);
            void destroy();
    };

    class VulkanTexture2D : public VulkanTexture {
        public:
            void loadFromKTXFile(
                    std::string filename,
                    VkFormat format,
                    VulkanDevice* device,
                    VkQueue copyQueue,
                    VkImageUsageFlags imageUsageFlag = VK_IMAGE_USAGE_SAMPLED_BIT,
                    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    bool forceLinear = false);

            void loadFromFile(
                    std::string filename,
                    VkFormat format,
                    VulkanDevice* device,
                    VkQueue copyQueue,
                    VkImageUsageFlags imageUsageFlag = VK_IMAGE_USAGE_SAMPLED_BIT,
                    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        private:
            void generateMipmaps(VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
            void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
            void copyBufferToImage(VulkanBuffer buffer, uint32_t width, uint32_t height);
            void createSampler();
    };

}
