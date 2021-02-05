#pragma once

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>
#include <glm/glm.hpp>
#include <math.h>

namespace VulkanLearning {

    enum Camera_Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UPWARD,
        DOWNWARD
    };

    const float YAW             = -90.0f;
    const float PITCH           = 0.0f;
    const float SPEED           = 2.5f;
    const float SENSITIVITY     = 0.1f;
    const float ZOOM            = 45.0f;

    class Camera {

        private:
            glm::vec3 m_Position;
            glm::vec3 m_Front;
            glm::vec3 m_Up;
            glm::vec3 m_Right;
            glm::vec3 m_WorldUp;

            float m_Yaw;
            float m_Pitch;

            float m_MovementSpeed;
            float m_MouseSensitivity;
            float m_Zoom;

            bool m_CaptureMouse = true;
            bool m_FirstMouse = true;
            float m_LastMouseX;
            float m_LastMouseY;

        public:
            Camera();

            Camera(glm::vec3 position,
                glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
                float yaw = YAW, float pitch = PITCH);

            Camera(float posX, float posY, float posZ, float upX, float upY, float upZ,
                    float yaw, float pitch);

            inline glm::vec3 position() { return m_Position; }
            inline void setPosition(glm::vec3 position) { 
                m_Position = position; 
                updateCameraVectors();
            }
            inline void setYaw(float yaw) {
                m_Yaw = yaw; 
                updateCameraVectors();
            }

            glm::mat4 getViewMatrix();
            float getZoom();

            void processZoom(double yOffset);
            void processMovement(Camera_Movement direction, float deltaTime);
            void processMouse(double xPos, double yPos, bool captureMouse);
            void toogleCaptureMouse(bool value);

            void printPosition();
            void print();

        private:
            void updateCameraVectors();
            void processMouseMovement(double xOffset, double yOffset, bool constrainPitch = true);
    };
}
