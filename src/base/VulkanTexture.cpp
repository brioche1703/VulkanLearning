#include "VulkanTexture.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanImageResource.hpp"
#include "ktx.h"

#include <cstring>

namespace VulkanLearning {

    VulkanTexture::VulkanTexture(std::string filename, VulkanDevice* device,
            VulkanSwapChain* swapChain,
            VulkanCommandPool* commandPool)
        : m_filename(filename), m_device(device), m_swapChain(swapChain), 
        m_commandPool(commandPool) {

            m_textureImageResources = new VulkanImageResource(m_device, 
                    m_swapChain, m_commandPool, VK_FORMAT_R8G8B8A8_SRGB, 
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | 
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
                    VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        }

    VulkanTexture::~VulkanTexture() {}

    void VulkanTexture::createKTX() {
        ktxTexture* texture;
        ktxResult res;

        res = ktxTexture_CreateFromNamedFile(m_filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
    }

    void VulkanTexture::create() {
        stbi_uc* pixels = stbi_load(m_filename.c_str(), &m_width, &m_height, 
                &m_channelCount, STBI_rgb_alpha);

        VkDeviceSize imageSize = m_width * m_height * 4;

        m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(
                            m_width, m_height)))) + 1;

        if (!pixels) {
            throw std::runtime_error("Texture image loading failed!");
        }

        VulkanBuffer stagingBuffer(m_device, m_commandPool);
        stagingBuffer.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 
                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data;
        vkMapMemory(m_device->getLogicalDevice(), 
                stagingBuffer.getBufferMemory(), 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(m_device->getLogicalDevice(), 
                stagingBuffer.getBufferMemory());

        stbi_image_free(pixels);

        m_textureImageResources->createImage(m_width, m_height, m_mipLevels, 
                VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, 
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT 
                | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        m_textureImageResources->transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, 
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                m_mipLevels);

        m_textureImageResources->copyBufferToImage(stagingBuffer,
                static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height));

        vkDestroyBuffer(m_device->getLogicalDevice(), stagingBuffer.getBuffer(), 
                nullptr);
        vkFreeMemory(m_device->getLogicalDevice(), 
                stagingBuffer.getBufferMemory(), nullptr);

        generateMipmaps(VK_FORMAT_R8G8B8A8_SRGB, m_width, m_height, m_mipLevels);
    }

    void VulkanTexture::cleanup() {
        vkDestroySampler(m_device->getLogicalDevice(), m_sampler, nullptr);
        m_textureImageResources->cleanup();
    }

    void VulkanTexture::generateMipmaps(VkFormat imageFormat, 
            int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(m_device->getPhysicalDevice(), 
                imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & 
                    VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("Image texture format does not support linear filtering");
        }

        VulkanCommandBuffer commandBuffer;
        commandBuffer.create(m_device, m_commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = m_textureImageResources->getImage();
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), 
                    VK_PIPELINE_STAGE_TRANSFER_BIT, 
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0, 0, nullptr, 0, nullptr, 1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, 
                mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer.getCommandBuffer(), 
                    m_textureImageResources->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                    m_textureImageResources->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, 
                    VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), 
                    VK_PIPELINE_STAGE_TRANSFER_BIT, 
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0, 0, nullptr, 0, nullptr, 1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), 
                VK_PIPELINE_STAGE_TRANSFER_BIT, 
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrier);

        commandBuffer.flushCommandBuffer(m_device, m_commandPool, true);
    }

    void VulkanTexture::transitionImageLayout(VkFormat format, 
            VkImageLayout oldLayout, VkImageLayout newLayout, 
            uint32_t mipLevels) {
        VulkanCommandBuffer commandBuffer;
        commandBuffer.create(m_device, m_commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_textureImageResources->getImage();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && 
                newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::runtime_error("Organisation of a transition not supported!");
        }

        vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), sourceStage, 
                destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        commandBuffer.flushCommandBuffer(m_device, m_commandPool, true);
    }

    void VulkanTexture::createImageView(VkFormat format, 
            VkImageAspectFlags aspectFlags) {
        m_textureImageResources->createImageView(format, aspectFlags, m_mipLevels);
    }


    void VulkanTexture::createSampler() {
        VkSamplerCreateInfo samplerInfo{};

        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0;
        /* samplerInfo.minLod = static_cast<float>(m_mipLevels / 2); */
        samplerInfo.maxLod = static_cast<float>(m_mipLevels);
        samplerInfo.mipLodBias = 0.0f;

        if (vkCreateSampler(m_device->getLogicalDevice(), &samplerInfo, 
                    nullptr, &m_sampler) != VK_SUCCESS) {
            throw std::runtime_error("Sampler creation failed!");
        }
    }
}
