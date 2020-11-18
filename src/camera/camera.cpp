#include "camera/camera.hpp"

#include "glm/ext/matrix_transform.hpp"

namespace VulkanLearning {

    Camera::Camera() {}

    Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) 
        : m_Front(glm::vec3(0.0f, 0.0f, -1.0f)), m_MovementSpeed(SPEED), 
        m_MouseSensitivity(SENSITIVITY), m_Zoom(ZOOM) {
        m_Position = position;
        m_WorldUp = up;
        m_Yaw = yaw;
        m_Pitch = pitch;
        updateCameraVectors();
    }

    Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ,
            float yaw, float pitch)            
        : m_Front(glm::vec3(0.0f, 0.0f, -1.0f)), m_MovementSpeed(SPEED),
        m_MouseSensitivity(SENSITIVITY), m_Zoom(ZOOM) {
            m_Position = glm::vec3(posX, posY, posZ);
            m_WorldUp = glm::vec3(upX, upY, upZ);
            m_Yaw = yaw;
            m_Pitch = pitch;
            updateCameraVectors();
        }

    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
    }

    float Camera::getZoom() {
        return m_Zoom;
    }

    void Camera::processMovement(Camera_Movement direction, float deltaTime) {
        float velocity = m_MovementSpeed * deltaTime;
        
        switch (direction) {
            case FORWARD:
                m_Position += m_Front * velocity;
                break;
            case BACKWARD:
                m_Position -= m_Front * velocity;
                break;
            case RIGHT:
                m_Position += m_Right * velocity;
                break;
            case LEFT:
                m_Position -= m_Right * velocity;
                break;
            case UPWARD:
                m_Position += m_Up * velocity;
                break;
            case DOWNWARD:
                m_Position -= m_Up * velocity;
                break;
        }
        updateCameraVectors();
    }

    void Camera::processZoom(double yOffset) {
        m_Zoom -= yOffset;
        if (m_Zoom < 1.0f) {
            m_Zoom = 1.0f;
        }
        if (m_Zoom > 45.0f) {
            m_Zoom = 45.0f;
        }
    }

    void Camera::processMouse(double xPos, double yPos, bool captureMouse) {
        toogleCaptureMouse(captureMouse);
        if (m_CaptureMouse) {
            if (m_FirstMouse) {
                m_LastMouseX = xPos;
                m_LastMouseY = yPos;
                m_FirstMouse = false;
            }

            float xOffset = xPos - m_LastMouseX;
            float yOffset = m_LastMouseY - yPos;

            m_LastMouseX = xPos;
            m_LastMouseY = yPos;

            processMouseMovement(xOffset, yOffset);
        }
    }

    void Camera::processMouseMovement(double xOffset, double yOffset, bool constrainPitch) {
        xOffset *= m_MouseSensitivity;
        yOffset *= m_MouseSensitivity;

        m_Yaw += xOffset;
        m_Pitch += yOffset;

        if (constrainPitch)
        {
            if (m_Pitch > 89.0f) {
                m_Pitch = 89.0f;
            }
            if (m_Pitch < -89.0f) {
                m_Pitch = -89.0f;
            }
        }

        updateCameraVectors();
    }

    void Camera::toogleCaptureMouse(bool value) {
        if (m_CaptureMouse != value)
            m_FirstMouse = true;
        m_CaptureMouse = value;
    }

    void Camera::updateCameraVectors() {
                glm::vec3 front;

                front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
                front.y = sin(glm::radians(m_Pitch)); 
                front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

                m_Front = glm::normalize(front);

                m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
                m_Up = glm::normalize(glm::cross(m_Right, m_Front));
    }

    void Camera::printPosition() {
        std::cout << "Camera Position : " 
            << m_Position.x << "  " 
            << m_Position.y << "  " 
            << m_Position.z << std::endl;
    }

    void Camera::print() {
        std::cout << "Camera Position : " 
            << m_Position.x << "  " 
            << m_Position.y << "  " 
            << m_Position.z << std::endl;
        std::cout << "Camera Front : " 
            << m_Front.x << "  " 
            << m_Front.y << "  " 
            << m_Front.z << std::endl;
                std::cout << m_Yaw << "   " << m_Pitch << std::endl;
    }
}
