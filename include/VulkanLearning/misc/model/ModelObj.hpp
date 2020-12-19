#pragma once

#define TINYOBJLOADER_IMPLEMENTATION

#include <string>
#include <vector>
#include <unordered_map>

#include "Vertex.hpp"

namespace VulkanLearning {

    class ModelObj {
        private:
            std::string m_modelPath;

            std::vector<VertexTextured> m_vertices;
            std::vector<uint32_t> m_indices;

        public:
            ModelObj();
            ModelObj(std::string modelPath);
            ~ModelObj();

            inline std::vector<VertexTextured> getVerticies() { return m_vertices; }
            inline std::vector<uint32_t> getIndicies() { return m_indices; }

            void load();
    };

}
