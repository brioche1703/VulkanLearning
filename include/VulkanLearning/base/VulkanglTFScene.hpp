#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

#include "VulkanBase.hpp"

namespace VulkanLearning {

    class VulkanglTFScene {
        public:
            VulkanDevice* device;
            VkQueue copyQueue;

            struct Vertex {
                glm::vec3 pos;
                glm::vec3 normal;
                glm::vec2 uv;
                glm::vec3 color;
                glm::vec4 tangent;
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
                std::string name;
                bool visible = true;
            };

            struct Material {
                glm::vec4 baseColorFactor = glm::vec4(1.0f);
                uint32_t baseColorTextureIndex;
                uint32_t normalTextureIndex;
                std::string alphaMode = "OPAQUE";
                float alphaCutOff;
                bool doubleSided = false;
                VkDescriptorSet descriptorSet;
                VkPipeline pipeline;
            };

            struct Image {
                VulkanTexture2D texture;
            };

            struct Texture {
                int32_t imageIndex;
            };

            std::vector<Image> images;
            std::vector<Texture> textures;
            std::vector<Material> materials;
            std::vector<Node> nodes;

            std::string path;

            VulkanglTFScene();
            ~VulkanglTFScene();

            VkDescriptorImageInfo getTextureDescriptor(const size_t index);

            void loadImages(tinygltf::Model& input);
            void loadTextures(tinygltf::Model& input);
            void loadMaterials(tinygltf::Model& input);
            void loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, VulkanglTFScene::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<VulkanglTFScene::Vertex>& vertexBuffer);
            void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VulkanglTFScene::Node node);
            void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

            void cleanup();
    };
}
