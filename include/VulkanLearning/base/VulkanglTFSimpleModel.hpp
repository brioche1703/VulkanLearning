#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include "VulkanBase.hpp"

namespace VulkanLearning {

    class VulkanglTFSimpleModel {
        public:
            VulkanDevice* device;
            VkQueue copyQueue;

            struct Vertex {
                glm::vec3 pos;
                glm::vec3 normal;
                glm::vec2 uv;
                glm::vec3 color;
            };

            struct {
                VulkanBuffer* buffer;
            } vertices;

            struct {
                int count;
                VulkanBuffer* buffer;
            } indices;

            struct Node;

            struct Primitive {
                uint32_t firstIndex;
                uint32_t indexCount;
                int32_t materialIndex;
            };

            struct Mesh {
                std::vector<Primitive> primitives;
            };

            struct Node {
                Node* parent;
                std::vector<Node> children;
                Mesh mesh;
                glm::mat4 matrix;
            };

            struct Material {
                glm::vec4 baseColorFactor = glm::vec4(1.0f);
                uint32_t baseColorTextureIndex;
            };

            struct Image {
                VulkanTexture2D texture;
                VkDescriptorSet descriptorSet;
            };

            struct Texture {
                int32_t imageIndex;
            };

            std::vector<Image> images;
            std::vector<Texture> textures;
            std::vector<Material> materials;
            std::vector<Node> nodes;

            VulkanglTFSimpleModel();
            ~VulkanglTFSimpleModel();

            void loadImages(tinygltf::Model& input);

            void loadTextures(tinygltf::Model& input);
            void loadMaterials(tinygltf::Model& input);
            void loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, VulkanglTFSimpleModel::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<VulkanglTFSimpleModel::Vertex>& vertexBuffer);

            void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VulkanglTFSimpleModel::Node node);

            void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
    };
}
