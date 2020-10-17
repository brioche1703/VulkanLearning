#pragma once

#include <array>
#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>

#include "VulkanDevice.hpp"
#include "VulkanSurface.hpp"
#include "../window/Window.hpp"

namespace VulkanLearning {

    struct FramebufferAttachment {
        VkImageView imageView;
    };

    class VulkanSwapChain {
        private:
            VkSwapchainKHR m_swapChain;

            std::vector<VkImage> m_images;
            std::vector<VkImageView> m_imagesViews;
            std::vector<VkFramebuffer> m_framebuffers;

            VkFormat m_imageFormat;
            VkExtent2D m_extent;

            Window* m_window;
            VulkanDevice* m_device;
            VulkanSurface* m_surface;

        public:
            VulkanSwapChain(Window* window, VulkanDevice* device,
                    VulkanSurface* surface);

            ~VulkanSwapChain();

            VkSwapchainKHR getSwapChain();
            std::vector<VkImage> getImages();
            std::vector<VkImageView> getImagesViews();
            std::vector<VkFramebuffer> getFramebuffers();
            VkFormat getImageFormat();
            VkExtent2D getExtent();

            void create();

            void createFramebuffers(VkRenderPass renderPass,
                    const std::vector<VkImageView> attachments);
            void createFramebuffersDeux(VkRenderPass renderPass,
                    const std::vector<VkImageView> attachments);

            void cleanFramebuffers();
            void destroyImageViews();
        private:

            VkSurfaceFormatKHR chooseSwapSurfaceFormat(
                    const std::vector<VkSurfaceFormatKHR>& availableFormats);
            VkPresentModeKHR chooseSwapPresentMode(
                    const std::vector<VkPresentModeKHR>& availablePresentModes);
            VkExtent2D chooseSwapExtent(
                    const VkSurfaceCapabilitiesKHR& capabilities);

            void createImageViews();
            VkImageView createImageView(
                    VkImage image, VkFormat format, 
                    VkImageAspectFlags aspectFlags, 
                    uint32_t mipLevels);

            void cleanup();
    };

}
