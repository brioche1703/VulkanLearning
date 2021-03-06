#pragma once

#include <string>
#include <algorithm>
#include <memory>
#include <cmath>

#include "ktx.h"

#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanImageResource.hpp"

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
            uint32_t m_layerCount;
            int m_channelCount;
            VkDescriptorImageInfo m_descriptor;
            VkSampler m_sampler;

        public:
            VulkanTexture() {}
            ~VulkanTexture() {}

            inline VkImage getImage() { return m_image; }
            inline VkImageView getView() { return m_view; }
            inline VkSampler getSampler() { return m_sampler; }
            inline VkImageLayout getImageLayout() { return m_imageLayout; }
            inline uint32_t getLayerCount() { return m_layerCount; }
            inline VkDescriptorImageInfo getDescriptor() { return m_descriptor; }
            inline VkDescriptorImageInfo* getDescriptorPointer() { return &m_descriptor; }

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

            void loadFromBuffer(
                    void* buffer,
                    VkDeviceSize bufferSize,
                    VkFormat format,
                    uint32_t texWidth,
                    uint32_t texHeight,
                    VulkanDevice* device,
                    VkQueue copyQueue,
                    VkFilter filter = VK_FILTER_LINEAR,
                    VkImageUsageFlags imageUsageFlag = VK_IMAGE_USAGE_SAMPLED_BIT,
                    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        private:
            void generateMipmaps(VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
            void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
            void copyBufferToImage(VulkanBuffer buffer, uint32_t width, uint32_t height);
            void createSampler();
    };

    class VulkanTexture2DArray : public VulkanTexture {
        public:
            void loadFromKTXFile(
                    std::string filename,
                    VkFormat format,
                    VulkanDevice* device,
                    VkQueue copyQueue,
                    VkImageUsageFlags imageUsageFlag = VK_IMAGE_USAGE_SAMPLED_BIT,
                    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    };

}
