#pragma once

#include <GLFW/glfw3.h>

#include "camera/camera.hpp"
#include "misc/FpsCounter.hpp"

namespace VulkanLearning {

    class Inputs {
        private:

        public:
            static bool m_captureMouse;
            static GLFWwindow* m_window;
            static Camera* m_camera;
            static FpsCounter* m_fpsCounter;

            Inputs(GLFWwindow* window, Camera* camera, FpsCounter* fpsCounter) {
                m_captureMouse = false;
                m_window = window;
                m_camera = camera;
                m_fpsCounter = fpsCounter;
            }

            ~Inputs();

            static void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
                if (key == GLFW_KEY_ESCAPE) {
                    if (action == GLFW_PRESS)
                        glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
                if (key == GLFW_KEY_M) {
                    if (action == GLFW_PRESS) {
                        m_captureMouse = !m_captureMouse;
                        if (m_captureMouse) {
                            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                        } else {
                            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                        }
                    }
                }
            }

            static void scroll_callback(GLFWwindow* window, double xOffset, double yOffset) {
                m_camera->processZoom(yOffset);
            }

            static void mouse_callback(GLFWwindow* window, double xPos, double yPos) {
                m_camera->processMouse(xPos, yPos, m_captureMouse);
            }

            void processKeyboardInput() {

                if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS && glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                    m_camera->processMovement(VulkanLearning::UPWARD, m_fpsCounter->getDeltaTime());
                } else if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) {
                    m_camera->processMovement(VulkanLearning::FORWARD, m_fpsCounter->getDeltaTime());
                }

                if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS && glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                    m_camera->processMovement(VulkanLearning::DOWNWARD, m_fpsCounter->getDeltaTime());
                } else if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) {
                    m_camera->processMovement(VulkanLearning::BACKWARD, m_fpsCounter->getDeltaTime());
                }

                if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) {
                    m_camera->processMovement(VulkanLearning::LEFT, m_fpsCounter->getDeltaTime());
                }

                if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) {
                    m_camera->processMovement(VulkanLearning::RIGHT, m_fpsCounter->getDeltaTime());
                }
            }
    };
}
