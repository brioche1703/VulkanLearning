#pragma once

#define TINYOBJLOADER_IMPLEMENTATION

#include <string>
#include <vector>
#include <unordered_map>

#include "../../base/Vertex.hpp"

namespace VulkanLearning {

    class ModelObj {
        private:
            std::string m_modelPath;

            std::vector<Vertex> m_vertices;
            std::vector<uint32_t> m_indices;

        public:
            ModelObj(std::string modelPath);
            ~ModelObj();

            inline std::vector<Vertex> getVerticies() { return m_vertices; }
            inline std::vector<uint32_t> getIndicies() { return m_indices; }

            void load();
    };

}
