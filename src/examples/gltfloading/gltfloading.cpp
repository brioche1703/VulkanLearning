#include "VulkanBase.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

namespace VulkanLearning {

    class VulkanglTFModel {
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


            VulkanglTFModel() {}
            ~VulkanglTFModel()
            {
                vkDestroyBuffer(device->getLogicalDevice(), vertices.buffer->getBuffer(), nullptr);
                vkFreeMemory(device->getLogicalDevice(), vertices.buffer->getBufferMemory(), nullptr);
                vkDestroyBuffer(device->getLogicalDevice(), indices.buffer->getBuffer(), nullptr);
                vkFreeMemory(device->getLogicalDevice(), indices.buffer->getBufferMemory(), nullptr);

                for (Image image : images) {
                    image.texture.destroy();
                }
            }

            inline VulkanDevice* getDevice() { return device; }

            void loadImages(tinygltf::Model& input) {
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
                            VK_FORMAT_R8G8B8A8_UNORM,
                            glTFImage.width,
                            glTFImage.height,
                            device,
                            copyQueue);

                    if (deleteBuffer) {
                        delete buffer;
                    }
                }
            }

            void loadTextures(tinygltf::Model& input) {
                textures.resize(input.textures.size());
                for (size_t i = 0; i < input.textures.size(); i++) {
                    textures[i].imageIndex = input.textures[i].source;
                }
            }

            void loadMaterials(tinygltf::Model& input) {
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

            void loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, VulkanglTFModel::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<VulkanglTFModel::Vertex>& vertexBuffer) {
                VulkanglTFModel::Node node{};
                node.matrix = glm::mat4(1.0f);

                if (inputNode.translation.size() == 3) {
                    node.matrix = glm::translate(node.matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
                }
                if (inputNode.rotation.size() == 3) {
                    glm::quat q = glm::make_quat(inputNode.rotation.data());
                    node.matrix *= glm::mat4(q);
                }
                if (inputNode.scale.size() == 3) {
                    node.matrix = glm::scale(node.matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
                }
                if (inputNode.children.size() == 16) {
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
                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                                uint32_t* buf = new uint32_t[accessor.count];
                                memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
                                for (size_t index = 0; index < accessor.count; index++) {
                                    indexBuffer.push_back(buf[index] + vertexStart);
                                }
                                break;
                            }

                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                                uint16_t* buf = new uint16_t[accessor.count];
                                memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
                                for (size_t index = 0; index < accessor.count; index++) {
                                    indexBuffer.push_back(buf[index] + vertexStart);
                                }
                                break;
                            }

                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  {
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

            void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VulkanglTFModel::Node node) {
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
                            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &images[texture.imageIndex].descriptorSet, 0, nullptr);
                            vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);

                        }
                    }
                }
                for (auto& child : node.children) {
                    drawNode(commandBuffer, pipelineLayout, child);
                }
            }

            void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
                VkDeviceSize offsets[1] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertices.buffer->getBufferPointer(), offsets);
                vkCmdBindIndexBuffer(commandBuffer, indices.buffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

                for (auto& node : nodes) {
                    drawNode(commandBuffer, pipelineLayout, node);
                }
            }
    };


    const std::string TEXTURE_PATH = "./src/textures/texturearray_rgba.ktx";

    class VulkanExample : public VulkanBase {

        private:

            VulkanglTFModel glTFModel;

            uint32_t m_msaaSamples = 64;

            struct UboInstanceData {
                glm::mat4 model;
                glm::vec4 arrayIndex;
            };

            struct DescriptorSetLayouts {
                VulkanDescriptorSetLayout* matrices;
                VulkanDescriptorSetLayout* textures;
            } m_descriptorSetLayouts;

        public:
            VulkanExample() {}
            ~VulkanExample() {}

            void run() {
                VulkanBase::run();
            }

        private:
            VulkanTexture2DArray m_texture;

            void initWindow() override {
                m_window = new Window("Vulkan", WIDTH, HEIGHT);
                m_window->init();
            }

            void initCore() override {
                m_camera = new Camera(glm::vec3(0.0f, 0.0f, 5.0f));
                m_fpsCounter = new FpsCounter();
                m_input = new Inputs(m_window->getWindow(), m_camera, m_fpsCounter);

                glfwSetKeyCallback(m_window->getWindow() , m_input->keyboard_callback);
                glfwSetScrollCallback(m_window->getWindow() , m_input->scroll_callback);
                glfwSetCursorPosCallback(m_window->getWindow() , m_input->mouse_callback);

                glfwSetInputMode(m_window->getWindow() , GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }

            void initVulkan() override {
                createInstance();
                createDebug();
                createSurface();
                createDevice();
                createSwapChain();
                createRenderPass();

                loadAssets();

                createDescriptorSetLayout();

                createGraphicsPipeline();

                createColorResources();
                createDepthResources();
                createFramebuffers();
                createTextureKTX();

                createVertexBuffer();
                createIndexBuffer();
                createCoordinateSystemUniformBuffers();
                createDescriptorPool();
                createDescriptorSets();
                createCommandBuffers();
                createSyncObjects();
            }

            void mainLoop() override {
                while (!glfwWindowShouldClose(m_window->getWindow() )) {
                    glfwPollEvents();
                    m_input->processKeyboardInput();
                    m_fpsCounter->update();
                    drawFrame();
                }

                vkDeviceWaitIdle(m_device->getLogicalDevice());
            }

            void drawFrame() override {
                vkWaitForFences(m_device->getLogicalDevice(), 1, 
                        &m_syncObjects->getInFlightFences()[currentFrame], VK_TRUE, UINT64_MAX);

                uint32_t imageIndex;

                VkResult result = vkAcquireNextImageKHR(m_device->getLogicalDevice(), 
                        m_swapChain->getSwapChain(), UINT64_MAX, 
                        m_syncObjects->getImageAvailableSemaphores()[currentFrame], 
                        VK_NULL_HANDLE, &imageIndex);

                if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                    recreateSwapChain();
                    return;
                } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                    throw std::runtime_error("Presentation of one image of the swap chain failed!");
                }

                if (m_syncObjects->getImagesInFlight()[imageIndex] != VK_NULL_HANDLE) {
                    vkWaitForFences(m_device->getLogicalDevice(), 1, &m_syncObjects->getImagesInFlight()[imageIndex], VK_TRUE, UINT64_MAX);
                }

                m_syncObjects->getImagesInFlight()[imageIndex] = m_syncObjects->getInFlightFences()[currentFrame];

                updateCamera(imageIndex);

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                VkSemaphore waitSemaphore[] = {m_syncObjects->getImageAvailableSemaphores()[currentFrame]};
                VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphore;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = m_commandBuffers[imageIndex].getCommandBufferPointer();

                VkSemaphore signalSemaphores[] = {m_syncObjects->getRenderFinishedSemaphores()[currentFrame]};
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                vkResetFences(m_device->getLogicalDevice(), 1, &m_syncObjects->getInFlightFences()[currentFrame]);

                if (vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submitInfo, m_syncObjects->getInFlightFences()[currentFrame]) != VK_SUCCESS) {
                    throw std::runtime_error("Command buffer sending failed!");
                }

                VkPresentInfoKHR presentInfo{};
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = signalSemaphores;

                VkSwapchainKHR swapChains[] = {m_swapChain->getSwapChain()};
                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = swapChains;
                presentInfo.pImageIndices = &imageIndex;
                presentInfo.pResults = nullptr;

                result = vkQueuePresentKHR(m_device->getPresentQueue(), &presentInfo);

                if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
                    framebufferResized = false;
                    recreateSwapChain();
                } else if (result != VK_SUCCESS) {
                    throw std::runtime_error("Presentation of one image of the swap chain failed!");
                }

                currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            }

            void cleanup() override {
                cleanupSwapChain();

                m_texture.destroy();

                m_descriptorSetLayout->cleanup();

                m_vertexBuffer->cleanup();
                m_indexBuffer->cleanup();

                m_syncObjects->cleanup();

                vkDestroyCommandPool(m_device->getLogicalDevice(), m_device->getCommandPool(), nullptr);

                vkDestroyDevice(m_device->getLogicalDevice(), nullptr);

                if (enableValidationLayers) {
                    m_debug->destroy(m_instance->getInstance(), nullptr);
                }

                vkDestroySurfaceKHR(m_instance->getInstance(), m_surface->getSurface(), nullptr);
                vkDestroyInstance(m_instance->getInstance(), nullptr);

                glfwDestroyWindow(m_window->getWindow() );

                glfwTerminate();
            }

            void createSurface() override {
                m_surface = new VulkanSurface(m_window, m_instance);
            }

            void recreateSwapChain() override {
                int width = 0, height = 0;
                while (width == 0 || height == 0) {
                    glfwGetFramebufferSize(m_window->getWindow() , &width, &height);
                    glfwWaitEvents();
                }

                vkDeviceWaitIdle(m_device->getLogicalDevice());

                cleanupSwapChain();

                m_swapChain->create();

                createRenderPass();
                createGraphicsPipeline();
                createColorResources();
                createDepthResources();

                createFramebuffers();

                createCoordinateSystemUniformBuffers();
                createDescriptorPool();
                createDescriptorSets();
                createCommandBuffers();
            }

            void cleanupSwapChain() override {
                m_colorImageResource->cleanup();
                m_depthImageResource->cleanup();

                m_swapChain->cleanFramebuffers();

                vkFreeCommandBuffers(m_device->getLogicalDevice(), 
                        m_device->getCommandPool(), 
                        static_cast<uint32_t>(
                            m_commandBuffers.size()), 
                        m_commandBuffers.data()->getCommandBufferPointer());

                vkDestroyPipeline(m_device->getLogicalDevice(), 
                        m_graphicsPipeline->getGraphicsPipeline(), nullptr);
                vkDestroyPipelineLayout(m_device->getLogicalDevice(), 
                        m_graphicsPipeline->getPipelineLayout(), nullptr);
                vkDestroyRenderPass(m_device->getLogicalDevice(), 
                        m_renderPass->getRenderPass(), nullptr);

                m_swapChain->destroyImageViews();
                vkDestroySwapchainKHR(m_device->getLogicalDevice(), m_swapChain->getSwapChain(), nullptr);

                for (size_t i = 0; i < m_swapChain->getImages().size(); i++) {
                    vkDestroyBuffer(m_device->getLogicalDevice(), 
                            m_coordinateSystemUniformBuffers[i]->getBuffer(), 
                            nullptr);
                    vkFreeMemory(m_device->getLogicalDevice(), 
                            m_coordinateSystemUniformBuffers[i]->getBufferMemory(), 
                            nullptr);
                }

                vkDestroyDescriptorPool(m_device->getLogicalDevice(), 
                        m_descriptorPool->getDescriptorPool(), nullptr);
            }

            void  createInstance() override {
                m_instance = new VulkanInstance(
                        "Texture Array",
                        enableValidationLayers, 
                        validationLayers, m_debug);
            }

            void  createDebug() override {
                m_debug = new VulkanDebug(m_instance->getInstance(), 
                        enableValidationLayers);
            }

            void  createDevice() override {
                m_device = new VulkanDevice(m_instance->getInstance(), m_surface->getSurface(), 
                        deviceExtensions,
                        enableValidationLayers, validationLayers, m_msaaSamples);
            }

            void  createSwapChain() override {
                m_swapChain = new VulkanSwapChain(m_window, m_device, m_surface);
            }

            void createRenderPass() override {
                VkAttachmentDescription colorAttachment{};
                colorAttachment.format = m_swapChain->getImageFormat();
                colorAttachment.samples = m_device->getMsaaSamples();
                colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentDescription depthAttachment{};
                depthAttachment.format = m_device->findDepthFormat();
                depthAttachment.samples = m_device->getMsaaSamples();
                depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentDescription colorAttachmentResolve{};
                colorAttachmentResolve.format = m_swapChain->getImageFormat();
                colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
                colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                VkAttachmentReference colorAttachmentRef{};
                colorAttachmentRef.attachment = 0;
                colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentReference depthAttachmentRef{};
                depthAttachmentRef.attachment = 1;
                depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentReference colorAttachmentResolveRef{};
                colorAttachmentResolveRef.attachment = 2;
                colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments = &colorAttachmentRef;
                subpass.pDepthStencilAttachment = &depthAttachmentRef;
                subpass.pResolveAttachments = &colorAttachmentResolveRef;

                const std::vector<VkAttachmentDescription> attachments = 
                { colorAttachment, depthAttachment, colorAttachmentResolve };

                m_renderPass = new VulkanRenderPass(m_swapChain, m_device);
                m_renderPass->create(attachments, subpass);
            }

            void createGraphicsPipeline() override {
                m_graphicsPipeline = new VulkanGraphicsPipeline(m_device,
                        m_swapChain, m_renderPass, m_descriptorSetLayout);

                VulkanShaderModule vertShaderModule = 
                    VulkanShaderModule("src/shaders/glTFLoadingVert.spv", m_device);
                VulkanShaderModule fragShaderModule = 
                    VulkanShaderModule("src/shaders/glTFLoadingFrag.spv", m_device);

                VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
                vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

                VkVertexInputBindingDescription vertexBindingDescription;
                vertexBindingDescription.binding = 0;
                vertexBindingDescription.stride = sizeof(VulkanglTFModel::Vertex);
                vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                std::vector<VkVertexInputAttributeDescription> vertexAttributeDescription;
                vertexAttributeDescription.resize(4);
                vertexAttributeDescription[0].binding = 0;
                vertexAttributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
                vertexAttributeDescription[0].location = 0;
                vertexAttributeDescription[0].offset = offsetof(VulkanglTFModel::Vertex, pos);

                vertexAttributeDescription[1].binding = 0;
                vertexAttributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
                vertexAttributeDescription[1].location = 1;
                vertexAttributeDescription[1].offset = offsetof(VulkanglTFModel::Vertex, normal);

                vertexAttributeDescription[2].binding = 0;
                vertexAttributeDescription[2].format = VK_FORMAT_R32G32B32_SFLOAT;
                vertexAttributeDescription[2].location = 2;
                vertexAttributeDescription[2].offset = offsetof(VulkanglTFModel::Vertex, uv);

                vertexAttributeDescription[3].binding = 0;
                vertexAttributeDescription[3].format = VK_FORMAT_R32G32B32_SFLOAT;
                vertexAttributeDescription[3].location = 3;
                vertexAttributeDescription[3].offset = offsetof(VulkanglTFModel::Vertex, color);

                vertexInputInfo.vertexBindingDescriptionCount = 1;
                vertexInputInfo.vertexAttributeDescriptionCount = 
                    static_cast<uint32_t>(vertexAttributeDescription.size());
                vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
                vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();

                VkPipelineDepthStencilStateCreateInfo depthStencil{};
                depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthStencil.depthTestEnable = VK_TRUE;
                depthStencil.depthWriteEnable = VK_TRUE;
                depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
                depthStencil.depthBoundsTestEnable = VK_FALSE;
                depthStencil.stencilTestEnable = VK_FALSE;

                std::array<VkDescriptorSetLayout, 2> setLayouts = { 
                    m_descriptorSetLayouts.matrices->getDescriptorSetLayout(), 
                    m_descriptorSetLayouts.textures->getDescriptorSetLayout() 
                };
                VkPushConstantRange pushConstantRange = {};
                pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                pushConstantRange.size = sizeof(glm::mat4);
                pushConstantRange.offset = 0;

                VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
                pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
                pipelineLayoutInfo.pSetLayouts = setLayouts.data();
                pipelineLayoutInfo.pushConstantRangeCount = 1;
                pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

                m_graphicsPipeline->create(
                        vertShaderModule, 
                        fragShaderModule,
                        vertexInputInfo, 
                        pipelineLayoutInfo, 
                        &depthStencil);
            }

            void createFramebuffers() override {
                const std::vector<VkImageView> attachments {
                    m_colorImageResource->getImageView(),
                        m_depthImageResource->getImageView()
                };

                m_swapChain->createFramebuffers(m_renderPass->getRenderPass(), 
                        attachments);
            }

            void createTextureKTX() {
                m_texture.loadFromKTXFile(TEXTURE_PATH, VK_FORMAT_R8G8B8A8_UNORM, m_device, m_device->getGraphicsQueue());
            }

            void createColorResources() override {
                m_colorImageResource = new VulkanImageResource(m_device, 
                        m_swapChain, 
                        m_swapChain->getImageFormat(),  
                        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
                        | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
                        VK_IMAGE_ASPECT_COLOR_BIT);
                m_colorImageResource->create();
            }

            void createDepthResources() override {
                m_depthImageResource = new VulkanImageResource(m_device, 
                        m_swapChain,
                        m_device->findDepthFormat(), 
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                        VK_IMAGE_ASPECT_DEPTH_BIT);
                m_depthImageResource->create();
            }

            void createCoordinateSystemUniformBuffers() override {
                VkDeviceSize bufferSize = sizeof(CoordinatesSystemUniformBufferObject);

                m_coordinateSystemUniformBuffers.resize(m_swapChain->getImages().size());

                for (size_t i = 0; i < m_swapChain->getImages().size(); i++) {
                    m_coordinateSystemUniformBuffers[i] = new VulkanBuffer(m_device);
                    m_coordinateSystemUniformBuffers[i]->createBuffer(
                            bufferSize, 
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                            *m_coordinateSystemUniformBuffers[i]->getBufferPointer(), 
                            *m_coordinateSystemUniformBuffers[i]->getBufferMemoryPointer());
                }
            }

            void createCommandBuffers() override {
                m_commandBuffers.resize(m_swapChain->getFramebuffers().size());

                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.commandPool = m_device->getCommandPool();
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocInfo.commandBufferCount = (uint32_t) m_commandBuffers.size();

                if (vkAllocateCommandBuffers(m_device->getLogicalDevice(), &allocInfo, m_commandBuffers.data()->getCommandBufferPointer()) != VK_SUCCESS) {
                    throw std::runtime_error("Command buffers allocation failed!");
                }

                std::array<VkClearValue, 2> clearValues{};
                clearValues[0].color = {0.25f, 0.25f, 0.25f, 1.0f};
                clearValues[1].depthStencil = {1.0f, 0};

                for (size_t i = 0; i < m_commandBuffers.size(); i++) {
                    VkCommandBufferBeginInfo beginInfo{};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = 0;
                    beginInfo.pInheritanceInfo = nullptr;

                    if (vkBeginCommandBuffer(m_commandBuffers[i].getCommandBuffer(), &beginInfo) != VK_SUCCESS) {
                        throw std::runtime_error("Begin recording of a command buffer failed!");
                    }

                    VkRenderPassBeginInfo renderPassInfo{};
                    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassInfo.renderPass = m_renderPass->getRenderPass(); 
                    renderPassInfo.framebuffer = m_swapChain->getFramebuffers()[i];
                    renderPassInfo.renderArea.offset = {0, 0};
                    renderPassInfo.renderArea.extent = m_swapChain->getExtent();
                    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
                    renderPassInfo.pClearValues = clearValues.data();

                    vkCmdBeginRenderPass(
                            m_commandBuffers[i].getCommandBuffer(), 
                            &renderPassInfo, 
                            VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdBindDescriptorSets(
                            m_commandBuffers[i].getCommandBuffer(),
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_graphicsPipeline->getPipelineLayout(),
                            0,
                            1,
                            &m_descriptorSets->getDescriptorSets()[i],
                            0,
                            nullptr);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_graphicsPipeline->getGraphicsPipeline());

                    glTFModel.draw(
                            m_commandBuffers[i].getCommandBuffer(), 
                            m_graphicsPipeline->getPipelineLayout());

                    vkCmdEndRenderPass(m_commandBuffers[i].getCommandBuffer());

                    if (vkEndCommandBuffer(m_commandBuffers[i].getCommandBuffer()) != VK_SUCCESS) {
                        throw std::runtime_error("Recording of a command buffer failed!");
                    }
                }
            }

            void createSyncObjects() override {
                m_syncObjects = new VulkanSyncObjects(m_device, m_swapChain, 
                        MAX_FRAMES_IN_FLIGHT);
            }

            void createDescriptorSetLayout() override {
                m_descriptorSetLayouts.matrices = new VulkanDescriptorSetLayout(m_device);
                m_descriptorSetLayouts.textures = new VulkanDescriptorSetLayout(m_device);

                VkDescriptorSetLayoutBinding uboLayoutBinding{};
                uboLayoutBinding.binding = 0;
                uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uboLayoutBinding.descriptorCount = 1;
                uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

                std::vector<VkDescriptorSetLayoutBinding> matricesDescriptorSetLayoutBinding= { uboLayoutBinding };
                m_descriptorSetLayouts.matrices->create(matricesDescriptorSetLayoutBinding);

                VkDescriptorSetLayoutBinding samplerLayoutBinding{};
                samplerLayoutBinding.binding = 0;
                samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                samplerLayoutBinding.descriptorCount = 1;
                samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                samplerLayoutBinding.pImmutableSamplers = nullptr;

                std::vector<VkDescriptorSetLayoutBinding> samplerDescriptorSetLayoutBinding = 
                { samplerLayoutBinding };

                m_descriptorSetLayouts.textures->create(samplerDescriptorSetLayoutBinding);

            }

            void createDescriptorPool() override {
                m_descriptorPool = new VulkanDescriptorPool(m_device, m_swapChain);

                std::vector<VkDescriptorPoolSize> poolSizes = std::vector<VkDescriptorPoolSize>(2);

                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = static_cast<uint32_t>(
                        m_swapChain->getImages().size());

                poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[1].descriptorCount = 
                    static_cast<uint32_t>(m_swapChain->getImages().size())
                            * static_cast<uint32_t>(glTFModel.images.size());

                const uint32_t maxSetCount = 
                    static_cast<uint32_t>(m_swapChain->getImages().size())
                            * static_cast<uint32_t>(glTFModel.images.size()) + 1;

                m_descriptorPool->create(poolSizes, maxSetCount);
            }

            void createDescriptorSets() override {
                m_descriptorSets = new VulkanDescriptorSets(
                        m_device, 
                        m_swapChain,
                        m_descriptorSetLayouts.matrices, 
                        m_descriptorPool);

                m_descriptorSets->create();

                std::vector<std::vector<VulkanBuffer*>> ubos{
                    m_coordinateSystemUniformBuffers
                };

                std::vector<VkDeviceSize> ubosSizes{
                    sizeof(CoordinatesSystemUniformBufferObject)
                };

                for (size_t i = 0; i < m_swapChain->getImages().size(); i++) {
                    std::vector<VkWriteDescriptorSet> descriptorWrites(1);

                    VkDescriptorBufferInfo bufferInfo{};
                    bufferInfo.offset = 0;
                    bufferInfo.buffer = ubos[0][i]->getBuffer();
                    bufferInfo.range = ubosSizes[0];

                    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrites[0].dstBinding = 0;
                    descriptorWrites[0].dstArrayElement = 0;
                    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    descriptorWrites[0].descriptorCount = 1;
                    descriptorWrites[0].pBufferInfo = &bufferInfo;

                    m_descriptorSets->update(descriptorWrites, i);
                }

                for (auto& image : glTFModel.images) {
                    VkDescriptorSetAllocateInfo allocInfo{};
                    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    allocInfo.descriptorPool = m_descriptorPool->getDescriptorPool();
                    allocInfo.pSetLayouts = m_descriptorSetLayouts.textures->getDescriptorSetLayoutPointer();
                    allocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapChain->getImages().size());

                    if (vkAllocateDescriptorSets(m_device->getLogicalDevice(), &allocInfo, &image.descriptorSet) != VK_SUCCESS) {
                        throw std::runtime_error("Descriptor set allocation failed!");
                    }

                    for (size_t i = 0; i < m_swapChain->getImages().size(); i++) {
                        VkWriteDescriptorSet descriptorWrite;
                        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        descriptorWrite.dstSet = image.descriptorSet;
                        descriptorWrite.dstBinding = 0;
                        descriptorWrite.dstArrayElement = 0;
                        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        descriptorWrite.descriptorCount = 1;
                        descriptorWrite.pImageInfo = image.texture.getDescriptorPointer();

                        vkUpdateDescriptorSets(m_device->getLogicalDevice(), 
                                1, 
                                &descriptorWrite, 
                                0, 
                                nullptr);
                    }
                }
            }

            void updateCamera(uint32_t currentImage) override {
                CoordinatesSystemUniformBufferObject ubo{};

                ubo.model = glm::mat4(1.0f);

                ubo.view = m_camera->getViewMatrix();

                ubo.proj = glm::perspective(glm::radians(m_camera->getZoom()), 
                        m_swapChain->getExtent().width / (float) m_swapChain->getExtent().height, 
                        0.1f,  100.0f);

                ubo.proj[1][1] *= -1;

                ubo.camPos = m_camera->position();

                m_coordinateSystemUniformBuffers[currentImage]->map();
                memcpy(m_coordinateSystemUniformBuffers[currentImage]->getMappedMemory(), 
                        &ubo, sizeof(ubo));
                m_coordinateSystemUniformBuffers[currentImage]->unmap();
            }

            void loadglTFFile(std::string filename) {
                tinygltf::Model glTFInput;
                tinygltf::TinyGLTF gltfContext;
                std::string error, warning;

                bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, filename);

                glTFModel.device = m_device;
                glTFModel.copyQueue = m_device->getGraphicsQueue();

                std::vector<uint32_t> indexBuffer;
                std::vector<VulkanglTFModel::Vertex> vertexBuffer;

                if (fileLoaded) {
                    glTFModel.loadImages(glTFInput);
                    glTFModel.loadMaterials(glTFInput);
                    glTFModel.loadTextures(glTFInput);

                    const tinygltf::Scene& scene = glTFInput.scenes[0];
                    for (size_t i = 0; i < scene.nodes.size(); i++) {
                        const tinygltf::Node node = glTFInput.nodes[scene.nodes[i]];
                        glTFModel.loadNode(node, glTFInput, nullptr, indexBuffer, vertexBuffer);
                    }
                } else {
                    std::runtime_error("glTF file loading failed!");
                }

                size_t vertexBufferSize = vertexBuffer.size() * sizeof(VulkanglTFModel::Vertex);
                size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
                glTFModel.indices.count = static_cast<uint32_t>(indexBuffer.size());

                VulkanBuffer vertexStagingBuffer(m_device);
                vertexStagingBuffer.createBuffer(
                        vertexBufferSize, 
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                VulkanBuffer indexStagingBuffer(m_device);
                indexStagingBuffer.createBuffer(
                        indexBufferSize, 
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                glTFModel.vertices.buffer = new VulkanBuffer(m_device);
                glTFModel.vertices.buffer->createBuffer(
                        vertexBufferSize, 
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                glTFModel.indices.buffer = new VulkanBuffer(m_device);
                glTFModel.indices.buffer->createBuffer(
                        indexBufferSize, 
                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                VulkanCommandBuffer copyCmd;
                copyCmd.create(m_device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

                VkBufferCopy copyRegion = {};
                copyRegion.size = vertexBufferSize;

                vkCmdCopyBuffer(
                        copyCmd.getCommandBuffer(), 
                        vertexStagingBuffer.getBuffer(),
                        glTFModel.vertices.buffer->getBuffer(),
                        1,
                        &copyRegion);

                copyRegion.size = indexBufferSize;

                vkCmdCopyBuffer(
                        copyCmd.getCommandBuffer(), 
                        indexStagingBuffer.getBuffer(),
                        glTFModel.indices.buffer->getBuffer(),
                        1,
                        &copyRegion);

                copyCmd.flushCommandBuffer(m_device, true);

                vertexStagingBuffer.cleanup();
                indexStagingBuffer.cleanup();
            }

            void loadAssets() {
                loadglTFFile("src/models/FlightHelmet/glTF/FlightHelmet.gltf");
            }


    };

}

VULKAN_EXAMPLE_MAIN()
