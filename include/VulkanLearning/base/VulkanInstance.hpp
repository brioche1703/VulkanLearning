#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cstring>

#include "../../../include/VulkanLearning/misc/VulkanDebug.hpp"

namespace VulkanLearning {

    class VulkanInstance {
        private:
            VkInstance m_instance;

            VulkanDebug* m_debug;

            const char* m_appName;
            bool m_enableValidationLayers;
            const std::vector<const char*> m_validationLayers;

        public:
            VulkanInstance();
            VulkanInstance(
                    const char* appName,
                    bool enableValidationLayers,
                    const std::vector<const char*> validationLayers,
                    VulkanDebug* debug);
            ~VulkanInstance();

            inline VkInstance getInstance() { return m_instance; }

        private:
            void create();
            bool checkValidationLayerSupport(
                    const std::vector<const char*> validationLayers);

            std::vector<const char*> getRequiredExtensions(
                    bool enableValidationLayers);
    };
}
