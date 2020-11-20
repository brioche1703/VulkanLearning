#include "VulkanImageResource.hpp"
#include "VulkanCommandBuffer.hpp"

namespace VulkanLearning {

    VulkanImageResource::VulkanImageResource(VulkanDevice* device, VulkanSwapChain* swapChain,
            VulkanCommandPool* commandPool, VkFormat format, 
            VkImageUsageFlags usage, VkImageAspectFlags aspect)
        : m_device(device), m_swapChain(swapChain), m_commandPool(commandPool), 
        m_format(format), m_usage(usage), m_aspect(aspect) {
        }

    VulkanImageResource::~VulkanImageResource() {}

    void VulkanImageResource::create() {
        createImage(m_swapChain->getExtent().width, 
                m_swapChain->getExtent().height, 1, 
                m_device->getMsaaSamples(), m_format, 
                VK_IMAGE_TILING_OPTIMAL, 
                m_usage,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        m_imageView = createImageView(m_image, m_format, m_aspect, 1);
    }

    void VulkanImageResource::cleanup() {
        vkDestroyImageView(m_device->getLogicalDevice(), m_imageView, nullptr);
        vkDestroyImage(m_device->getLogicalDevice(), m_image, nullptr);
        vkFreeMemory(m_device->getLogicalDevice(), m_imageMemory, nullptr);
    }

    void VulkanImageResource::createImage(uint32_t width, 
            uint32_t height, uint32_t mipLevels, 
            VkSampleCountFlagBits numSamples, VkFormat format, 
            VkImageTiling tiling, VkImageUsageFlags usage, 
            VkMemoryPropertyFlags properties) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.samples = numSamples;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(m_device->getLogicalDevice(), &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
            throw std::runtime_error("Image creation failed!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_device->getLogicalDevice(), m_image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_device->findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_device->getLogicalDevice(), &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("Memory allocation for an image failed!");
        }

        vkBindImageMemory(m_device->getLogicalDevice(), m_image, m_imageMemory, 0);
    }

    VkImageView VulkanImageResource::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(m_device->getLogicalDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("Image view creation failed!");
        }

        return imageView;
    }

    void VulkanImageResource::createImageView(VkFormat format, 
            VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device->getLogicalDevice(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
            throw std::runtime_error("Image view creation failed!");
        }
    }

    void VulkanImageResource::transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
        VulkanCommandBuffer commandBuffer;
        commandBuffer.create(m_device, m_commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

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

        commandBuffer.flushCommandBuffer(m_device, m_commandPool, true);
    }
}
