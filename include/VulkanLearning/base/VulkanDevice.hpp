#pragma once

#include <vulkan/vulkan.h>

#include <iostream>

#include <optional>
#include <vector>

namespace VulkanLearning {

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    class VulkanDevice {
        private:
            VkPhysicalDevice m_physicalDevice;
            VkDevice m_logicalDevice;

            VkQueue m_graphicsQueue;
            VkQueue m_presentQueue;

            std::vector<const char*> m_deviceExtensions; 
            VkSampleCountFlagBits m_msaaSamples;

            QueueFamilyIndices m_queueFamilyIndices;

        public:
            VulkanDevice();
            VulkanDevice(VkInstance instance, VkSurfaceKHR surface,
                    const std::vector<const char*> deviceExtensions,
                    bool enableValidationLayers,
                    const std::vector<const char*> validationLayers);
            ~VulkanDevice();

            VkPhysicalDevice getPhysicalDevice();
            VkDevice getLogicalDevice();
            VkQueue getGraphicsQueue();
            VkQueue getPresentQueue();
            VkSampleCountFlagBits getMsaaSamples();
            QueueFamilyIndices getQueueFamilyIndices();
            

            void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const std::vector<const char*> deviceExtensions);
            void createLogicalDevice(VkSurfaceKHR surface, bool enableValidationLayers, 
                    const std::vector<const char*> validationLayers);

            SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface); 
            SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface); 
            QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

            VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, 
                    VkImageTiling tiling, VkFormatFeatureFlags features);
            VkFormat findDepthFormat();

        private:
            bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*> deviceExtensions);
            VkSampleCountFlagBits getMaxUsableSampleCount(); 
            bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*> deviceExtensions);

    };
}
