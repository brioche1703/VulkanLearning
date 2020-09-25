#include "../../include/VulkanLearning/base/VulkanSurface.hpp"

#include <iostream>

namespace VulkanLearning {

    VulkanSurface::VulkanSurface(GLFWwindow* window, VkInstance instance) {
        createSurface(window, instance);
    }
    VulkanSurface::~VulkanSurface() {}

    void VulkanSurface::createSurface(GLFWwindow* window, VkInstance instance) {
        if (glfwCreateWindowSurface(instance, window, 
                    nullptr, &m_surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    VkSurfaceKHR VulkanSurface::getSurface() { return m_surface; }
}
