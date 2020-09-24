#pragma once

#include <vulkan/vulkan_core.h>

#include <iostream>

namespace VulkanLearning {

    class VulkanDebug {
        private:
            VkDebugUtilsMessengerEXT m_debugMessenger;

        public:
            VulkanDebug();
            VulkanDebug(VkInstance instance, bool enableValidationLayers);

            ~VulkanDebug();

            void setup(VkInstance instance, bool enableValidationLayers);

            VkResult create(VkInstance instance, 
                    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
                    const VkAllocationCallbacks* pAllocator);

            void populate(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

            void destroy(VkInstance instance, 
                    const VkAllocationCallbacks* pAllocator);

            static VKAPI_ATTR VkBool32 VKAPI_CALL 
            debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageType, 
                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
                    void* pUserData) {
                std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

                return VK_FALSE;
            }

    };
}
