#include "VulkanSurface.hpp"

#include <iostream>

namespace VulkanLearning {
    VulkanSurface::VulkanSurface() {}
    VulkanSurface::~VulkanSurface() {}

    void VulkanSurface::create(Window window, VulkanInstance instance) {
            VK_CHECK_RESULT(glfwCreateWindowSurface(instance.getInstance(), window.getWindow(), nullptr, &m_surface));
    }

}

