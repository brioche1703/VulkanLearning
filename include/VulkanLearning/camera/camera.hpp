#pragma once

#include <iostream>
#include <glm/glm.hpp>

namespace VulkanLearning {

    class Camera {

        private:
            glm::vec3 position;

        public:
            inline Camera() {}

            Camera(glm::vec3 position);
            void print();
    };
}
