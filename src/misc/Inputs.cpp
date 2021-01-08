#include "misc/Inputs.hpp"

namespace VulkanLearning {
    Camera* Inputs::m_camera = new Camera(glm::vec3(0.0f, 0.0f, 5.0f));
    FpsCounter* Inputs::m_fpsCounter = new FpsCounter();
    bool Inputs::m_captureMouse = false;
    GLFWwindow* Inputs::m_window = nullptr;
    UI* Inputs::m_ui = nullptr;
    Inputs::MousePos* Inputs::m_mousePos = new MousePos();
    Inputs::MouseButtons* Inputs::m_mouseButtons = new MouseButtons();
}
