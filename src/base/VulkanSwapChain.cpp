#include "../../include/VulkanLearning/base/VulkanSwapChain.hpp"

namespace VulkanLearning {

    VulkanSwapChain::VulkanSwapChain(Window* window, VulkanDevice* device,
            VulkanSurface* surface) 
        : m_window(window), m_device(device), m_surface(surface) {
        create();
    }

    VulkanSwapChain::~VulkanSwapChain() {}

    VkSwapchainKHR VulkanSwapChain::getSwapChain() { return m_swapChain; }
    std::vector<VkImage> VulkanSwapChain::getImages() { return m_images; }
    std::vector<VkImageView> VulkanSwapChain::getImagesViews() { return m_imagesViews; }
    std::vector<VkFramebuffer> VulkanSwapChain::getFramebuffers() { return m_framebuffers; }
    VkFormat VulkanSwapChain::getImageFormat() { return m_imageFormat; }
    VkExtent2D VulkanSwapChain::getExtent() { return m_extent; }

    void VulkanSwapChain::create() {
        SwapChainSupportDetails swapChainSupport = m_device->querySwapChainSupport(m_surface->getSurface());

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        if (swapChainSupport.capabilities.maxImageCount > 0 && 
                imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_surface->getSurface();

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = m_device->getQueueFamilyIndices();

        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_device->getLogicalDevice(), &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
            throw std::runtime_error("Swap Chain creation failed!");
        }

        vkGetSwapchainImagesKHR(m_device->getLogicalDevice(), m_swapChain, &imageCount, nullptr);
        m_images.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device->getLogicalDevice(), m_swapChain, &imageCount, m_images.data());

        m_imageFormat = surfaceFormat.format;
        m_extent = extent;

        createImageViews();
    }

    void VulkanSwapChain::createImageViews() {
        m_imagesViews.resize(m_images.size());

        for (size_t i = 0; i < m_images.size(); i++) {
            m_imagesViews[i] = createImageView(m_images[i], m_imageFormat, 
                    VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    VkImageView VulkanSwapChain::createImageView(VkImage image, VkFormat format, 
            VkImageAspectFlags aspectFlags, 
            uint32_t mipLevels) {
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
    
    void VulkanSwapChain::createFramebuffers(VkRenderPass renderPass,
            const std::vector<VkImageView> attachments) {
        m_framebuffers.resize(m_imagesViews.size());

        for (size_t i = 0; i < m_imagesViews.size(); i++) {
            std::vector<VkImageView> attachmentsAux = attachments;
            attachmentsAux.push_back(m_imagesViews[i]);
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentsAux.size());
            framebufferInfo.pAttachments = attachmentsAux.data();
            framebufferInfo.width = m_extent.width;
            framebufferInfo.height = m_extent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_device->getLogicalDevice(), 
                        &framebufferInfo, nullptr, &m_framebuffers[i]) 
                    != VK_SUCCESS) {;
                throw std::runtime_error("A framebuffer creation failed!");
            }
        }
    }

    VkSurfaceFormatKHR VulkanSwapChain::chooseSwapSurfaceFormat(
            const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
                    availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR VulkanSwapChain::chooseSwapPresentMode(
            const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapChain::chooseSwapExtent(
            const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(m_window->getWindow(), &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    void VulkanSwapChain::cleanup() {
    }

    void VulkanSwapChain::cleanFramebuffers() {
        for (auto framebuffer : m_framebuffers) {
            vkDestroyFramebuffer(m_device->getLogicalDevice(), framebuffer, nullptr);
        }
    }

    void VulkanSwapChain::destroyImageViews() {
        for (auto imageView : m_imagesViews) {
            vkDestroyImageView(m_device->getLogicalDevice(), imageView, nullptr);
        }
    }
}
