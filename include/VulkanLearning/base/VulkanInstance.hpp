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

        public:
            VulkanInstance();
            VulkanInstance(bool enableValidationLayers,
                    const std::vector<const char*> validationLayers,
                    VulkanDebug debug);
            ~VulkanInstance();

            inline VkInstance getInstance() { return m_instance; }

        private:
            void create(bool enableValidationLayers, 
                    const std::vector<const char*> validationLayers,
                    VulkanDebug debug);
            bool checkValidationLayerSupport(
                    const std::vector<const char*> validationLayers);
            std::vector<const char*> getRequiredExtensions(
                    bool enableValidationLayers);
    };
}
