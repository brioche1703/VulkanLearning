#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "VulkanInstance.hpp"
#include "window/Window.hpp"

namespace VulkanLearning {
    class VulkanSurface {

        private:
            VkSurfaceKHR m_surface;

            Window* m_window;
            VulkanInstance* m_instance;

        public:
            VulkanSurface(Window* window, VulkanInstance* instance);
            ~VulkanSurface();

            inline VkSurfaceKHR getSurface() { return m_surface; }

        private:
            void create();
    };
}
