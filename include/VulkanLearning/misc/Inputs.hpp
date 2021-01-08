#pragma once

#include <GLFW/glfw3.h>

#include "Camera.hpp"
#include "FpsCounter.hpp"
#include "UI.hpp"

namespace VulkanLearning {

    class Inputs {
        private:

        public:
            static bool m_captureMouse;
            static GLFWwindow* m_window;
            static Camera* m_camera;
            static FpsCounter *m_fpsCounter;
            static UI *m_ui;
            
            struct MousePos {
                double x;
                double y;
            };
            struct MouseButtons {
                bool left = false;
                bool right = false;
                bool middle = false;
            };
            static MousePos *m_mousePos;
            static MouseButtons *m_mouseButtons;

            Inputs() {};

            Inputs(GLFWwindow* window, Camera* camera, FpsCounter* fpsCounter, UI* ui) {
                m_captureMouse = false;
                m_window = window;
                m_camera = camera;
                m_fpsCounter = fpsCounter;
                m_ui = ui;
            }

            ~Inputs() {};

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
                if (key == GLFW_KEY_F1) {
                    if (action == GLFW_PRESS) {
                        m_ui->visible = !m_ui->visible;
                        //m_ui->updated = true;
                    }
                }
            }

            static void scroll_callback(GLFWwindow* window, double xOffset, double yOffset) {
                m_camera->processZoom(yOffset);
            }

            static void mouse_callback(GLFWwindow* window, double xPos, double yPos) {
                if (m_captureMouse) {
                    m_camera->processMouse(xPos, yPos, m_captureMouse);
                }
            }

            static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
                if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
                    glfwGetCursorPos(window, &m_mousePos->x, &m_mousePos->y);
                    std::cout << "Left mouse pressed : " << m_mousePos->x << "  :  "<< m_mousePos->y << std::endl;
                    m_mouseButtons->left = true;
                }
                 if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
                    glfwGetCursorPos(window, &m_mousePos->x, &m_mousePos->y);
                    std::cout << "Left mouse released : " << m_mousePos->x << "  :  "<< m_mousePos->y << std::endl;
                    m_mouseButtons->left = false;
                }
                if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
                    glfwGetCursorPos(window, &m_mousePos->x, &m_mousePos->y);
                    std::cout << "Right mouse pressed : " << m_mousePos->x << "  :  "<< m_mousePos->y << std::endl;
                    m_mouseButtons->right = true;
                }
                if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
                    glfwGetCursorPos(window, &m_mousePos->x, &m_mousePos->y);
                    std::cout << "Middle mouse pressed : " << m_mousePos->x << "  :  "<< m_mousePos->y << std::endl;
                    m_mouseButtons->middle = true;
                }
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
