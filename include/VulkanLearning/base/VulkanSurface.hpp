#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace VulkanLearning {
    class VulkanSurface {

        private:
            VkSurfaceKHR m_surface;

        public:
            VulkanSurface(GLFWwindow* window, VkInstance instance);
            ~VulkanSurface();


            VkSurfaceKHR getSurface();

        private:
            void createSurface(GLFWwindow* window, VkInstance instance);
    };
}
