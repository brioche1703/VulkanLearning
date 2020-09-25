#pragma once

#include <array>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>

#include "VulkanDevice.hpp"


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

        public:
            VulkanSwapChain(GLFWwindow* window, VulkanDevice device,
                    VkSurfaceKHR surface);

            ~VulkanSwapChain();

            VkSwapchainKHR getSwapChain();
            std::vector<VkImage> getImages();
            std::vector<VkImageView> getImagesViews();
            std::vector<VkFramebuffer> getFramebuffers();
            VkFormat getImageFormat();
            VkExtent2D getExtent();

            void create(GLFWwindow* window, VulkanDevice device, 
                    VkSurfaceKHR surface);

            void createFramebuffers(VkDevice device, VkRenderPass renderPass,
                    const std::vector<VkImageView> attachments);
        private:

            VkSurfaceFormatKHR chooseSwapSurfaceFormat(
                    const std::vector<VkSurfaceFormatKHR>& availableFormats);
            VkPresentModeKHR chooseSwapPresentMode(
                    const std::vector<VkPresentModeKHR>& availablePresentModes);
            VkExtent2D chooseSwapExtent(GLFWwindow* window,
                    const VkSurfaceCapabilitiesKHR& capabilities);

            void createImageViews(VkDevice device);
            VkImageView createImageView(VkDevice device, 
                    VkImage image, VkFormat format, 
                    VkImageAspectFlags aspectFlags, 
                    uint32_t mipLevels);

    };

}
