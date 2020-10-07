#include "../../include/VulkanLearning/base/VulkanSurface.hpp"

#include <iostream>

namespace VulkanLearning {

    VulkanSurface::VulkanSurface(Window* window, VulkanInstance* instance)
        : m_window(window), m_instance(instance) {
        create();
    }

    VulkanSurface::~VulkanSurface() {}

    void VulkanSurface::create() {
        if (glfwCreateWindowSurface(m_instance->getInstance(), 
                    m_window->getWindow(), 
                    nullptr, &m_surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

}
