#include "../../include/VulkanLearning/misc/Inputs.hpp"

namespace VulkanLearning {
    Camera* Inputs::m_camera = new Camera(glm::vec3(0.0f, 0.0f, 5.0f));
    FpsCounter* Inputs::m_fpsCounter = new FpsCounter();
    bool Inputs::m_captureMouse = false;
    GLFWwindow* Inputs::m_window = nullptr;
}
