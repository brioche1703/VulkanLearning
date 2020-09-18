#include <GLFW/glfw3.h>

namespace VulkanLearning {
    class FpsCounter {
        private:
            float m_deltaTime;
            float m_lastFrameTime;
            float m_currentFrameTime;

        public:
            FpsCounter(float deltaTime = 0.0f, float lastFrameTime = 0.0f, float currentFrameTime = 0.0f)
                : m_deltaTime(deltaTime), m_lastFrameTime(lastFrameTime), m_currentFrameTime(currentFrameTime) {
            }

            void update() {
                m_currentFrameTime = glfwGetTime();
                m_deltaTime = m_currentFrameTime - m_lastFrameTime;
                m_lastFrameTime = m_currentFrameTime;
            }

            inline float getDeltaTime() { return m_deltaTime; }
    };
}
