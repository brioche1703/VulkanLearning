#pragma once

#include <vulkan/vulkan_core.h>
#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include <array>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

struct VertexTextured {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(VertexTextured);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescription;
        attributeDescription[0].binding = 0;
        attributeDescription[0].location = 0;
        attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription[0].offset = offsetof(VertexTextured, pos);

        attributeDescription[1].binding = 0;
        attributeDescription[1].location = 1;
        attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription[1].offset = offsetof(VertexTextured, color);

        attributeDescription[2].binding = 0;
        attributeDescription[2].location = 2;
        attributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription[2].offset = offsetof(VertexTextured, texCoord);

        attributeDescription[3].binding = 0;
        attributeDescription[3].location = 3;
        attributeDescription[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription[3].offset = offsetof(VertexTextured, normal);

        return attributeDescription;
    }

    bool operator==(const VertexTextured& other) const {
        return 
            pos == other.pos 
            && color == other.color 
            && texCoord == other.texCoord
            && normal == other.normal;
    }
};

namespace std {
    template<> struct hash<VertexTextured> {
        size_t operator()(VertexTextured const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) 
                        ^ (hash<glm::vec3>()(vertex.color) << 1 )) >> 1) 
                        ^ (hash<glm::vec2>()(vertex.texCoord) << 1)
                        ^ (hash<glm::vec3>()(vertex.normal) << 1);
        }
    };
}
