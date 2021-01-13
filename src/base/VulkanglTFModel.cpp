#include "VulkanglTFModel.hpp"

namespace VulkanLearning {
    VulkanglTFModel::VulkanglTFModel() {}

    VulkanglTFModel::~VulkanglTFModel()
    {
        vertices.buffer->cleanup();
        indices.buffer->cleanup();

        for (Image image : images) {
            image.texture.destroy();
        }
    }

    void VulkanglTFModel::loadImages(tinygltf::Model& input) {
        images.resize(input.images.size());
        for (size_t i = 0; i < input.images.size(); i++) {
            tinygltf::Image& glTFImage = input.images[i];
            unsigned char* buffer = nullptr;
            VkDeviceSize bufferSize = 0;
            bool deleteBuffer = false;

            if (glTFImage.component == 3) {
                bufferSize = glTFImage.width * glTFImage.height * 4;
                buffer = new unsigned char[bufferSize];
                unsigned char* rgba = buffer;
                unsigned char* rgb = &glTFImage.image[0];
                for (size_t i = 0; i < glTFImage.width * glTFImage.height; ++i) {
                    memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                    rgba += 4;
                    rgb += 3;
                }
                deleteBuffer = true;
            } else {
                buffer = &glTFImage.image[0];
                bufferSize = glTFImage.image.size();
            }

            images[i].texture.loadFromBuffer(
                    buffer, 
                    bufferSize,
                    VK_FORMAT_R8G8B8A8_SRGB,
                    glTFImage.width,
                    glTFImage.height,
                    device,
                    copyQueue);

            if (deleteBuffer) {
                delete buffer;
            }
        }
    }

    void VulkanglTFModel::loadTextures(tinygltf::Model& input) {
        textures.resize(input.textures.size());
        for (size_t i = 0; i < input.textures.size(); i++) {
            textures[i].imageIndex = input.textures[i].source;
        }
    }

    void VulkanglTFModel::loadMaterials(tinygltf::Model& input) {
        materials.resize(input.materials.size());
        for (size_t i = 0; i < input.materials.size(); i++) {
            tinygltf::Material glTFMaterial = input.materials[i];
            if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) {
                materials[i].baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
            }
            if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) {
                materials[i].baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
            }
        }
    }

    void VulkanglTFModel::loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, VulkanglTFModel::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<VulkanglTFModel::Vertex>& vertexBuffer) {
        VulkanglTFModel::Node node{};
        node.matrix = glm::mat4(1.0f);

        if (inputNode.translation.size() == 3) {
            node.matrix = glm::translate(node.matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
        }
        if (inputNode.rotation.size() == 4) {
            glm::quat q = glm::make_quat(inputNode.rotation.data());
            node.matrix *= glm::mat4(q);
        }
        if (inputNode.scale.size() == 3) {
            node.matrix = glm::scale(node.matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
        }
        if (inputNode.matrix.size() == 16) {
            node.matrix = glm::make_mat4x4(inputNode.matrix.data());
        }

        if (inputNode.children.size() > 0) {
            for (size_t i = 0; i < inputNode.children.size(); i++) {
                loadNode(input.nodes[inputNode.children[i]], input, &node, indexBuffer, vertexBuffer);
            }
        }

        if (inputNode.mesh > -1) {
            const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
            for (size_t i = 0; i < mesh.primitives.size(); i++) {
                const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
                uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
                uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
                uint32_t indexCount = 0;

                // Vertices
                const float* positionBuffer = nullptr;
                const float* normalsBuffer = nullptr;
                const float* texCoordsBuffer = nullptr;
                size_t vertexCount = 0;

                if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
                    const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                    positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    vertexCount = accessor.count;
                }
                if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                    normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }
                if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                    texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }

                for (size_t v = 0; v < vertexCount; v++) {
                    Vertex vert{};
                    vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
                    vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                    vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
                    vert.color = glm::vec3(1.0f);
                    vertexBuffer.push_back(vert);
                }


                // Indices
                const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
                const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

                indexCount += static_cast<uint32_t>(accessor.count);

                switch (accessor.componentType) {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                                                                   uint32_t* buf = new uint32_t[accessor.count];
                                                                   memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
                                                                   for (size_t index = 0; index < accessor.count; index++) {
                                                                       indexBuffer.push_back(buf[index] + vertexStart);
                                                                   }
                                                                   break;
                                                               }

                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                                                                     uint16_t* buf = new uint16_t[accessor.count];
                                                                     memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
                                                                     for (size_t index = 0; index < accessor.count; index++) {
                                                                         indexBuffer.push_back(buf[index] + vertexStart);
                                                                     }
                                                                     break;
                                                                 }

                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:  {
                                                                     uint8_t* buf = new uint8_t[accessor.count];
                                                                     memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
                                                                     for (size_t index = 0; index < accessor.count; index++) {
                                                                         indexBuffer.push_back(buf[index] + vertexStart);
                                                                     }
                                                                     break;
                                                                 }

                    default:
                                                                 std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                                                                 return;

                }

                Primitive primitive{};
                primitive.firstIndex = firstIndex;
                primitive.indexCount = indexCount;
                primitive.materialIndex = glTFPrimitive.material;
                node.mesh.primitives.push_back(primitive);
            }
        }

        if (parent) {
            parent->children.push_back(node);
        } else {
            nodes.push_back(node);
        }
    }

    void VulkanglTFModel::drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VulkanglTFModel::Node node) {
        if (node.mesh.primitives.size() > 0) {
            glm::mat4 nodeMatrix = node.matrix;
            VulkanglTFModel::Node* currentParent = node.parent;
            while (currentParent) {
                nodeMatrix = currentParent->matrix * nodeMatrix;
                currentParent = currentParent->parent;
            }

            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);
            for (VulkanglTFModel::Primitive& primitive : node.mesh.primitives) {
                if (primitive.indexCount > 0) {
                    VulkanglTFModel::Texture texture = textures[materials[primitive.materialIndex].baseColorTextureIndex];
                    vkCmdBindDescriptorSets(commandBuffer, 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            pipelineLayout, 
                            1, 
                            1, 
                            &images[texture.imageIndex].descriptorSet, 
                            0, 
                            nullptr);
                    vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);

                }
            }
        }
        for (auto& child : node.children) {
            drawNode(commandBuffer, pipelineLayout, child);
        }
    }

    void VulkanglTFModel::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertices.buffer->getBufferPointer(), offsets);
        vkCmdBindIndexBuffer(commandBuffer, indices.buffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

        for (auto& node : nodes) {
            drawNode(commandBuffer, pipelineLayout, node);
        }
    }
}
