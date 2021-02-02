#pragma once

#include <GLFW/glfw3.h>
#include <string>

namespace VulkanLearning {
    class Window {
        private:
            GLFWwindow* m_window; 
            std::string m_title;
            uint32_t m_width;
            uint32_t m_height;

            static bool m_framebufferResized;
        public:
            Window() {};
            Window(std::string title, uint32_t width, uint32_t height) {
                m_title = title;
                m_width = width;
                m_height = height;
                m_framebufferResized = false;
            }
            ~Window() {}

            inline GLFWwindow* getWindow() { return m_window; }
            inline std::string getTitle() { return m_title; }
            inline void setTitle(std::string title) { 
                m_title = title;
                glfwSetWindowTitle(m_window, title.c_str());
            }

            void init() {
                glfwInit();

                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
                glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

                m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
                glfwSetWindowUserPointer(m_window, static_cast<void *>(this));
                glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
            }

            static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
                m_framebufferResized = true;
            }
    };
}
