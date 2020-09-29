#include "../../include/VulkanLearning/base/VulkanImageResource.hpp"

namespace VulkanLearning {

    VulkanImageResource::VulkanImageResource(VulkanDevice* device, VulkanSwapChain* swapChain,
            VkFormat format, VkImageUsageFlags usage, 
            VkImageAspectFlags aspect)
        : m_device(device), m_swapChain(swapChain), m_format(format),
        m_usage(usage), m_aspect(aspect) {
            create();
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
        allocInfo.memoryTypeIndex = m_device->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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
}
