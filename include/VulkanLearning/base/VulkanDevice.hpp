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
            uint32_t m_msaaSamplesMax;

            QueueFamilyIndices m_queueFamilyIndices;

            VkCommandPool m_commandPool = VK_NULL_HANDLE;

        public:
            VkPhysicalDeviceFeatures features;
            VkPhysicalDeviceFeatures enabledFeatures = {};
            VkPhysicalDeviceProperties properties;

            VulkanDevice() {};
            VulkanDevice(VkInstance instance, 
                    VkSurfaceKHR surface,
                    const std::vector<const char*> deviceExtensions,
                    bool enableValidationLayers,
                    const std::vector<const char*> validationLayers,
                    uint32_t msaaSamplesMax);
            VulkanDevice(uint32_t msaaSamplesMax);

            ~VulkanDevice();

            VkPhysicalDevice getPhysicalDevice();
            VkDevice getLogicalDevice();
            VkQueue getGraphicsQueue();
            VkQueue getPresentQueue();
            VkSampleCountFlagBits getMsaaSamples();
            size_t getMinUniformBufferOffsetAlignment();
            QueueFamilyIndices getQueueFamilyIndices();
            VkCommandPool getCommandPool();

            void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const std::vector<const char*> deviceExtensions);
            void createLogicalDevice(VkSurfaceKHR surface, bool enableValidationLayers, const std::vector<const char*> validationLayers);

            SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface); 
            SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface); 
            QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

            VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, 
                    VkImageTiling tiling, VkFormatFeatureFlags features);
            VkFormat findDepthFormat();
            uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        private:
            bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*> deviceExtensions);
            VkSampleCountFlagBits getMaxUsableSampleCount(); 
            bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*> deviceExtensions);

            bool isSamplerAnisotropySupported();

            void createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    };
}
