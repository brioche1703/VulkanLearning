#include "../../include/VulkanLearning/camera/camera.hpp"

namespace VulkanLearning {

    Camera::Camera(glm::vec3 position) {
        position = position;
    }

    void Camera::print() {
        std::cout << "camera!" << std::endl;
    }

}
