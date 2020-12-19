#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "VulkanInstance.hpp"
#include "VulkanTools.hpp"
#include "window/Window.hpp"

namespace VulkanLearning {
    class VulkanSurface {

        private:
            VkSurfaceKHR m_surface;

        public:
            VulkanSurface();
            ~VulkanSurface();

            inline VkSurfaceKHR getSurface() { return m_surface; }

            void create(Window window, VulkanInstance instance);
    };
}
