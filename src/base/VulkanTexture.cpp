#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "VulkanBase.hpp"

namespace VulkanLearning {

    ktxResult VulkanTexture::loadKTXFile(std::string filename, ktxTexture **target) {
        ktxResult result = KTX_SUCCESS;

        result = ktxTexture_CreateFromNamedFile(
                filename.c_str(), 
                KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, 
                target);

        if (result != KTX_SUCCESS) {
            throw std::runtime_error("KTX Texture :" + filename + " creation failed!");
        }

        return result;
    }

    void VulkanTexture::destroy() {
        vkDestroyImageView(m_device->getLogicalDevice(), m_view, nullptr);
        vkDestroyImage(m_device->getLogicalDevice(), m_image, nullptr);

        if (m_sampler) {
            vkDestroySampler(m_device->getLogicalDevice(), m_sampler, nullptr);
        }
        vkFreeMemory(m_device->getLogicalDevice(), m_deviceMemory, nullptr);
    }

    void VulkanTexture2D::loadFromKTXFile(std::string filename, VkFormat format, VulkanDevice* device, VkQueue copyQueue, VkImageUsageFlags imageUsageFlag, VkImageLayout imageLayout, bool forceLinear) {
        m_device = device;

        ktxResult result;
        ktxTexture* ktxTexture;

        result = loadKTXFile(filename, &ktxTexture);

        m_width = ktxTexture->baseWidth;
        m_height = ktxTexture->baseHeight;
        m_mipLevels = ktxTexture->numLevels;

        ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);
        ktx_size_t ktxTextureSize = ktxTexture_GetDataSize(ktxTexture);

        bool forceLinearTiling = true;
        if (forceLinearTiling) {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(m_device->getPhysicalDevice(), format, &formatProperties);
        }

        VkMemoryAllocateInfo memAllocInfo = {};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

        VkMemoryRequirements memReqs = {};

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;

        VkBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = ktxTextureSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_device->getLogicalDevice(), &bufferCreateInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Buffer creation failed!");
        }

        vkGetBufferMemoryRequirements(m_device->getLogicalDevice(), stagingBuffer, &memReqs);
        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex = m_device->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(m_device->getLogicalDevice(), &memAllocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
            throw std::runtime_error("Memory allocation failed!");
        }

        if (vkBindBufferMemory(m_device->getLogicalDevice(), stagingBuffer, stagingMemory, 0) != VK_SUCCESS) {
            throw std::runtime_error("Buffer memory binding failed!");
        }

        uint8_t *data;
        if (vkMapMemory(m_device->getLogicalDevice(), stagingMemory, 0, memReqs.size, 0, (void **)&data) != VK_SUCCESS) {
            throw std::runtime_error("Mapping memory failed!");
        }
        memcpy(data, ktxTextureData, ktxTextureSize);
        vkUnmapMemory(m_device->getLogicalDevice(), stagingMemory);

        std::vector<VkBufferImageCopy> bufferCopyRegions;

        uint32_t offset = 0;

        for (uint32_t i = 0; i < m_mipLevels; i++) {
            ktx_size_t offset;
            if (ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset) != KTX_SUCCESS) {
                throw std::runtime_error("KTX get image offset failed!");
            }

            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = i;
            bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> i;
            bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> i;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = offset;
            bufferCopyRegions.push_back(bufferCopyRegion);
        }

        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.mipLevels = m_mipLevels;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.extent = { (uint32_t)m_width, (uint32_t)m_height, 1 };
        imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        if (vkCreateImage(m_device->getLogicalDevice(), &imageCreateInfo, nullptr, &m_image) != VK_SUCCESS) {
            throw std::runtime_error("Image creation failed!");
        }

        vkGetImageMemoryRequirements(m_device->getLogicalDevice(), m_image, &memReqs);
        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex = m_device->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(m_device->getLogicalDevice(), &memAllocInfo, nullptr, &m_deviceMemory) != VK_SUCCESS) {
            throw std::runtime_error("Memory allocation failed!");
        }

        if (vkBindImageMemory(m_device->getLogicalDevice(), m_image, m_deviceMemory, 0) != VK_SUCCESS) {
            throw std::runtime_error("Image memory binding failed!");
        }

        VulkanCommandBuffer copyCmd;
        copyCmd.create(m_device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = m_mipLevels;
        subresourceRange.layerCount = 1;

        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.image = m_image;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        vkCmdPipelineBarrier(
                copyCmd.getCommandBuffer(), 
                VK_PIPELINE_STAGE_HOST_BIT, 
                VK_PIPELINE_STAGE_TRANSFER_BIT, 
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &imageMemoryBarrier);

        vkCmdCopyBufferToImage(
                copyCmd.getCommandBuffer(), 
                stagingBuffer, 
                m_image, 
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                static_cast<uint32_t>(bufferCopyRegions.size()), 
                bufferCopyRegions.data());

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        vkCmdPipelineBarrier(
                copyCmd.getCommandBuffer(), 
                VK_PIPELINE_STAGE_TRANSFER_BIT, 
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &imageMemoryBarrier);

        m_imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        copyCmd.flushCommandBuffer(m_device, true);

        vkFreeMemory(m_device->getLogicalDevice(), stagingMemory, nullptr);
        vkDestroyBuffer(m_device->getLogicalDevice(), stagingBuffer, nullptr);

        ktxTexture_Destroy(ktxTexture);

        VkSamplerCreateInfo sampler = {};
        sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler.magFilter = VK_FILTER_LINEAR;
        sampler.minFilter = VK_FILTER_LINEAR;
        sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.mipLodBias = 0.0f;
        sampler.compareOp = VK_COMPARE_OP_NEVER;
        sampler.minLod = 0.0f;
        sampler.maxLod = (float)m_mipLevels;
        if (m_device->features.samplerAnisotropy) {
            sampler.anisotropyEnable = VK_TRUE;
            sampler.maxAnisotropy = 16.0f;
        } else {
            sampler.anisotropyEnable = VK_FALSE;
            sampler.maxAnisotropy = 1.0f;
        }

        sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        if (vkCreateSampler(m_device->getLogicalDevice(), &sampler, 
                    nullptr, &m_sampler) != VK_SUCCESS) {
            throw std::runtime_error("Sampler creation failed!");
        }

        VkImageViewCreateInfo view = {};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = format;
        view.components = {
            VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A
        };
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.baseMipLevel = 0;
        view.subresourceRange.baseArrayLayer = 0;
        view.subresourceRange.layerCount = 1;
        view.subresourceRange.levelCount = m_mipLevels;
        view.image = m_image;

        if (vkCreateImageView(m_device->getLogicalDevice(), &view, 
                    nullptr, &m_view) != VK_SUCCESS) {
            throw std::runtime_error("Image view creation failed!");
        }

        m_descriptor.sampler = m_sampler;
        m_descriptor.imageView = m_view;
        m_descriptor.imageLayout = m_imageLayout;
    }


    void VulkanTexture2D::loadFromFile(std::string filename, VkFormat format, VulkanDevice* device, VkQueue copyQueue, VkImageUsageFlags imageUsageFlag, VkImageLayout imageLayout) {
        m_device = device;
        stbi_uc* pixels = stbi_load(filename.c_str(), &m_width, &m_height, &m_channelCount, STBI_rgb_alpha);

        VkDeviceSize imageSize = m_width * m_height * 4;

        m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(
                            m_width, m_height)))) + 1;

        if (!pixels) {
            throw std::runtime_error("Texture image loading failed!");
        }

        VulkanBuffer stagingBuffer(*m_device);
        stagingBuffer.createBuffer(
                imageSize, 
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data;
        vkMapMemory(m_device->getLogicalDevice(), 
                stagingBuffer.getBufferMemory(), 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(m_device->getLogicalDevice(), 
                stagingBuffer.getBufferMemory());

        stbi_image_free(pixels);

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = m_width;
        imageInfo.extent.height = m_height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = m_mipLevels;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(m_device->getLogicalDevice(), &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
            throw std::runtime_error("Image creation failed!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_device->getLogicalDevice(), m_image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_device->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(m_device->getLogicalDevice(), &allocInfo, nullptr, &m_deviceMemory) != VK_SUCCESS) {
            throw std::runtime_error("Memory allocation for an image failed!");
        }

        vkBindImageMemory(m_device->getLogicalDevice(), m_image, m_deviceMemory, 0);

        transitionImageLayout(
                VK_FORMAT_R8G8B8A8_SRGB, 
                VK_IMAGE_LAYOUT_UNDEFINED, 
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                m_mipLevels);

        copyBufferToImage(
                stagingBuffer, 
                static_cast<uint32_t>(m_width), 
                static_cast<uint32_t>(m_height));

        vkDestroyBuffer(
                m_device->getLogicalDevice(), 
                stagingBuffer.getBuffer(), 
                nullptr);

        vkFreeMemory(
                m_device->getLogicalDevice(), 
                stagingBuffer.getBufferMemory(), 
                nullptr);

        generateMipmaps(VK_FORMAT_R8G8B8A8_SRGB, m_width, m_height, m_mipLevels);

        createSampler();

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = m_mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device->getLogicalDevice(), &viewInfo, nullptr, &m_view) != VK_SUCCESS) {
            throw std::runtime_error("Image view creation failed!");
        }
        
        m_descriptor.sampler = m_sampler;
        m_descriptor.imageView = m_view;
        m_descriptor.imageLayout = m_imageLayout;
    }

    void VulkanTexture2D::loadFromBuffer(void* buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t texWidth, uint32_t texHeight, VulkanDevice* device, VkQueue copyQueue, VkFilter filter, VkImageUsageFlags imageUsageFlag, VkImageLayout imageLayout) {
        if (buffer == nullptr) {
            throw std::runtime_error("Buffer is null in texture loading from buffer!");
        }

        m_device = device;
        m_width = texWidth;
        m_height = texHeight;
        m_mipLevels = 1;
    
        VkMemoryAllocateInfo memAllocInfo = {};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        VkMemoryRequirements memReqs;

        VulkanCommandBuffer copyCmd;
        copyCmd.create(device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VulkanBuffer stagingBuffer(*m_device);
        stagingBuffer.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        
        stagingBuffer.map(bufferSize);
        memcpy(stagingBuffer.getMappedMemory(), buffer, bufferSize);
        stagingBuffer.unmap();

        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = 0;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = m_width;
        bufferCopyRegion.imageExtent.height = m_height;
        bufferCopyRegion.imageExtent.depth = 1;
        bufferCopyRegion.bufferOffset = 0;

        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
         imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = m_mipLevels;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.extent = { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height), 1 };
		imageCreateInfo.usage = imageUsageFlag;
        if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if (vkCreateImage(m_device->getLogicalDevice(), &imageCreateInfo, nullptr, &m_image) != VK_SUCCESS) {
            throw std::runtime_error("Image creation failed!");
        }

        vkGetImageMemoryRequirements(m_device->getLogicalDevice(), m_image, &memReqs);
        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex = m_device->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(m_device->getLogicalDevice(), &memAllocInfo, nullptr, &m_deviceMemory) != VK_SUCCESS) {
            throw std::runtime_error("Memory allocation failed!");
        }

        if (vkBindImageMemory(m_device->getLogicalDevice(), m_image, m_deviceMemory, 0) != VK_SUCCESS) {
            throw std::runtime_error("Image memory binding failed!");
        }

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = m_mipLevels;
		subresourceRange.layerCount = 1;

        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.image = m_image;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        vkCmdPipelineBarrier(
                copyCmd.getCommandBuffer(), 
                VK_PIPELINE_STAGE_HOST_BIT, 
                VK_PIPELINE_STAGE_TRANSFER_BIT, 
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &imageMemoryBarrier);

        vkCmdCopyBufferToImage(
                copyCmd.getCommandBuffer(), 
                stagingBuffer.getBuffer(), 
                m_image, 
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                1,
                &bufferCopyRegion);

        m_imageLayout = imageLayout;

        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = imageLayout;

        vkCmdPipelineBarrier(
                copyCmd.getCommandBuffer(), 
                VK_PIPELINE_STAGE_HOST_BIT, 
                VK_PIPELINE_STAGE_TRANSFER_BIT, 
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &imageMemoryBarrier);

        copyCmd.flushCommandBuffer(m_device, false);

        stagingBuffer.cleanup();

        VkSamplerCreateInfo sampler = {};
        sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler.magFilter = filter;
        sampler.minFilter = filter;
        sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.mipLodBias = 0.0f;
        sampler.compareOp = VK_COMPARE_OP_NEVER;
        sampler.minLod = 0.0f;
        sampler.maxLod = 0.0f;
        sampler.maxAnisotropy = 1.0f;

        if (vkCreateSampler(m_device->getLogicalDevice(), &sampler, 
                    nullptr, &m_sampler) != VK_SUCCESS) {
            throw std::runtime_error("Sampler creation failed!");
        }

        VkImageViewCreateInfo view = {};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.pNext = NULL;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = format;
        view.components = {
            VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A
        };
        view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        view.subresourceRange.levelCount = 1;
        view.image = m_image;

        if (vkCreateImageView(m_device->getLogicalDevice(), &view, 
                    nullptr, &m_view) != VK_SUCCESS) {
            throw std::runtime_error("Image view creation failed!");
        }

        m_descriptor.sampler = m_sampler;
        m_descriptor.imageView = m_view;
        m_descriptor.imageLayout = m_imageLayout;
    }

    void VulkanTexture2D::generateMipmaps(VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(m_device->getPhysicalDevice(), 
                imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & 
                    VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("Image texture format does not support linear filtering");
        }

        VulkanCommandBuffer commandBuffer;
        commandBuffer.create(m_device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = m_image;
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
                    m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                    m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, 
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

        commandBuffer.flushCommandBuffer(m_device, true);
    }

    void VulkanTexture2D::transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
        VulkanCommandBuffer commandBuffer;
        commandBuffer.create(m_device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_image;
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
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::runtime_error("Organisation of a transition not supported!");
        }

        vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), sourceStage, 
                destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        commandBuffer.flushCommandBuffer(m_device, true);
    }

    void VulkanTexture2D::copyBufferToImage(VulkanBuffer buffer, uint32_t width, uint32_t height) {
        VulkanCommandBuffer commandBuffer;
        commandBuffer.create(m_device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

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

        vkCmdCopyBufferToImage(
                commandBuffer.getCommandBuffer(), 
                buffer.getBuffer(), 
                m_image, 
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                1, 
                &region);

        commandBuffer.flushCommandBuffer(m_device, true);

    }

    void VulkanTexture2D::createSampler() {
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

    void VulkanTexture2DArray::loadFromKTXFile(std::string filename, VkFormat format, VulkanDevice* device, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout) {
        m_device = device;

        ktxTexture* ktxTexture;
        ktxResult result = loadKTXFile(filename, &ktxTexture);

        m_width = ktxTexture->baseWidth;
        m_height = ktxTexture->baseHeight;
        m_layerCount = ktxTexture->numLayers;
        m_mipLevels = ktxTexture->numLevels;

        ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);
        ktx_size_t ktxTextureSize = ktxTexture_GetDataSize(ktxTexture);

        VkMemoryAllocateInfo memAllocInfo = {};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        VkMemoryRequirements memReqs;

        VulkanBuffer stagingBuffer(*m_device);
        stagingBuffer.createBuffer(ktxTextureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        stagingBuffer.map(ktxTextureSize);
        memcpy(stagingBuffer.getMappedMemory(), ktxTextureData, ktxTextureSize);
        stagingBuffer.unmap();

        std::vector<VkBufferImageCopy> bufferCopyRegions;

        for (uint32_t layer = 0; layer < m_layerCount; layer++) {
             for (uint32_t level = 0; level < m_mipLevels; level++) {
                ktx_size_t offset;
                KTX_error_code result = ktxTexture_GetImageOffset(
                        ktxTexture, 
                        level, 
                        layer, 
                        0, 
                        &offset);

                VkBufferImageCopy bufferCopyRegion = {};
                bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                bufferCopyRegion.imageSubresource.mipLevel = level;
                bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
                bufferCopyRegion.imageSubresource.layerCount = 1;
                bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> level;
                bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> level;
                bufferCopyRegion.imageExtent.depth = 1;
                bufferCopyRegion.bufferOffset = offset;

                bufferCopyRegions.push_back(bufferCopyRegion);
             }
        }

        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.extent = { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height), 1 };
        imageCreateInfo.usage = imageUsageFlags;
        if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        imageCreateInfo.arrayLayers = m_layerCount;
        imageCreateInfo.mipLevels = m_mipLevels;

        if (vkCreateImage(m_device->getLogicalDevice(), &imageCreateInfo, nullptr, &m_image) != VK_SUCCESS) {
            throw std::runtime_error("Image creation failed!");
        }

        vkGetImageMemoryRequirements(m_device->getLogicalDevice(), m_image, &memReqs);
        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex = m_device->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(m_device->getLogicalDevice(), &memAllocInfo, nullptr, &m_deviceMemory) != VK_SUCCESS) {
            throw std::runtime_error("Memory allocation failed!");
        }

        if (vkBindImageMemory(m_device->getLogicalDevice(), m_image, m_deviceMemory, 0) != VK_SUCCESS) {
            throw std::runtime_error("Image memory binding failed!");
        }

        VulkanCommandBuffer copyCmd;
        copyCmd.create(m_device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = m_mipLevels;
        subresourceRange.layerCount = m_layerCount;

        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.image = m_image;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        vkCmdPipelineBarrier(
                copyCmd.getCommandBuffer(), 
                VK_PIPELINE_STAGE_HOST_BIT, 
                VK_PIPELINE_STAGE_TRANSFER_BIT, 
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &imageMemoryBarrier);

        vkCmdCopyBufferToImage(
                copyCmd.getCommandBuffer(), 
                stagingBuffer.getBuffer(), 
                m_image, 
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                static_cast<uint32_t>(bufferCopyRegions.size()), 
                bufferCopyRegions.data());

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        vkCmdPipelineBarrier(
                copyCmd.getCommandBuffer(), 
                VK_PIPELINE_STAGE_TRANSFER_BIT, 
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &imageMemoryBarrier);

        m_imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        copyCmd.flushCommandBuffer(m_device, true);

        VkSamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.mipLodBias = 0.0f;
        if (m_device->features.samplerAnisotropy) {
            samplerCreateInfo.anisotropyEnable = VK_TRUE;
            samplerCreateInfo.maxAnisotropy = 16.0f;
        } else {
            samplerCreateInfo.anisotropyEnable = VK_FALSE;
            samplerCreateInfo.maxAnisotropy = 1.0f;
        }
        samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerCreateInfo.minLod = 0.0f;
        samplerCreateInfo.maxLod = (float)m_mipLevels;
        samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        if (vkCreateSampler(m_device->getLogicalDevice(), &samplerCreateInfo, 
                    nullptr, &m_sampler) != VK_SUCCESS) {
            throw std::runtime_error("Sampler creation failed!");
        }
        
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewCreateInfo.format = format;
        viewCreateInfo.components = {
            VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A,
        };
        viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0 ,1, 0, 1 };
        viewCreateInfo.subresourceRange.layerCount = m_layerCount;
        viewCreateInfo.subresourceRange.levelCount = m_mipLevels;
        viewCreateInfo.image = m_image;

        if (vkCreateImageView(m_device->getLogicalDevice(), &viewCreateInfo, 
                    nullptr, &m_view) != VK_SUCCESS) {
            throw std::runtime_error("Image view creation failed!");
        }

        stagingBuffer.cleanup();
        ktxTexture_Destroy(ktxTexture);

        m_descriptor.sampler = m_sampler;
        m_descriptor.imageView = m_view;
        m_descriptor.imageLayout = m_imageLayout;
    }
}
