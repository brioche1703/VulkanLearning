#include <unordered_map>
#include <cmath>
#include <functional>
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include "../include/external/stb/stb_image.h"
#include "../include/external/tinyobjloader/tiny_obj_loader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtx/hash.hpp>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>
#include <array>
#include <unordered_map>
#include <chrono>

#include "../include/VulkanLearning/camera/camera.hpp"
#include "../include/VulkanLearning/misc/FpsCounter.hpp"
#include "../include/VulkanLearning/misc/VulkanDebug.hpp"
#include "../include/VulkanLearning/base/VulkanDevice.hpp"
#include "../include/VulkanLearning/base/VulkanInstance.hpp"
#include "../include/VulkanLearning/base/VulkanSwapChain.hpp"
#include "../include/VulkanLearning/base/VulkanSurface.hpp"
#include "../include/VulkanLearning/base/VulkanRenderPass.hpp"

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescription;
        attributeDescription[0].binding = 0;
        attributeDescription[0].location = 0;
        attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription[0].offset = offsetof(Vertex, pos);

        attributeDescription[1].binding = 0;
        attributeDescription[1].location = 1;
        attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription[1].offset = offsetof(Vertex, color);

        attributeDescription[2].binding = 0;
        attributeDescription[2].location = 2;
        attributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription[2].offset = offsetof(Vertex, texCoord);

        return attributeDescription;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                        (hash<glm::vec3>()(vertex.color) << 1 )) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct CoordinatesUniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};


namespace VulkanLearning {


    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    const std::string MODEL_PATH = "./src/models/viking_room.obj";
    const std::string TEXTURE_PATH = "./src/textures/viking_room.png";

    const int MAX_FRAMES_IN_FLIGHT = 2;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    VulkanLearning::Camera cam(glm::vec3(0.0f, 0.0f, 5.0f));
    static bool captureMouse = true;
    static VulkanLearning::FpsCounter fpsCounter;

    class HelloTriangleApplication {
        public:
            void run() {
                initWindow();
                initVulkan();
                mainLoop();
                cleanup();
            }

        private:
            GLFWwindow* window;

            VulkanInstance *m_instance;
            VulkanDebug* m_debug = new VulkanDebug();
            VulkanDevice* m_device;
            VulkanSurface* m_surface;

            VulkanSwapChain *m_swapChain;

            VulkanRenderPass* m_renderPass;

            VkDescriptorSetLayout descriptorSetLayout;
            VkDescriptorPool descriptorPool;
            std::vector<VkDescriptorSet> descriptorSets;
            VkPipelineLayout pipelineLayout;

            VkPipeline graphicsPipeline;

            VkCommandPool commandPool;

            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            VkBuffer vertexBuffer;
            VkDeviceMemory vertexBufferMemory;
            VkBuffer indexBuffer;
            VkDeviceMemory indexBufferMemory;
            std::vector<VkBuffer> uniformBuffers;
            std::vector<VkDeviceMemory> uniformBuffersMemory;

            std::vector<VkCommandBuffer> commandBuffers;

            std::vector<VkSemaphore> imageAvailableSemaphores;
            std::vector<VkSemaphore> renderFinishedSemaphores;
            std::vector<VkFence> inFlightFences;
            std::vector<VkFence> imagesInFlight;
            size_t currentFrame = 0;

            bool framebufferResized = false;

            uint32_t mipLevels;
            VkImage textureImage;
            VkDeviceMemory textureImageMemory;
            VkImageView textureImageView;

            VkSampler textureSampler;

            VkImage depthImage;
            VkDeviceMemory depthImageMemory;
            VkImageView depthImageView;

            VkImage colorImage;
            VkDeviceMemory colorImageMemory;
            VkImageView colorImageView;

            void initWindow() {
                glfwInit();

                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
                glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

                window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
                glfwSetWindowUserPointer(window, this);
                glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

                glfwSetKeyCallback(window, keyboard_callback);
                glfwSetScrollCallback(window, scroll_callback);
                glfwSetCursorPosCallback(window, mouse_callback);

                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }

            static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
                auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
                app->framebufferResized = true;
            }

            void initVulkan() {
                createInstance();
                createDebug();
                createSurface();
                createDevice();
                createSwapChain();
                createRenderPass();
                createDescriptorSetLayout();
                createGraphicsPipeline();
                createCommandPool();
                createColorResources();
                createDepthResources();
                createFramebuffers();
                createTextureImage();
                createTextureImageView();
                createTextureSampler();
                loadModel();
                createVertexBuffer();
                createIndexBuffer();
                createUniformBuffers();
                createDescriptorPool();
                createDescriptorSets();
                createCommandBuffers();
                createSyncObjects();
            }

            void mainLoop() {
                while (!glfwWindowShouldClose(window)) {
                    glfwPollEvents();
                    processKeyboardInput();
                    fpsCounter.update();
                    drawFrame();
                }

                vkDeviceWaitIdle(m_device->getLogicalDevice());
            }

            void drawFrame() {
                vkWaitForFences(m_device->getLogicalDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

                uint32_t imageIndex;

                VkResult result = vkAcquireNextImageKHR(m_device->getLogicalDevice(), m_swapChain->getSwapChain(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

                if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                    recreateSwapChain();
                    return;
                } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                    throw std::runtime_error("Presentation of one image of the swap chain failed!");
                }

                if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
                    vkWaitForFences(m_device->getLogicalDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
                }

                imagesInFlight[imageIndex] = inFlightFences[currentFrame];

                updateCamera(imageIndex);

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                VkSemaphore waitSemaphore[] = {imageAvailableSemaphores[currentFrame]};
                VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphore;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

                VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                vkResetFences(m_device->getLogicalDevice(), 1, &inFlightFences[currentFrame]);

                if (vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
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

            void cleanup() {
                cleanupSwapChain();

                vkDestroySampler(m_device->getLogicalDevice(), textureSampler, nullptr);
                vkDestroyImageView(m_device->getLogicalDevice(), textureImageView, nullptr);

                vkDestroyImage(m_device->getLogicalDevice(), textureImage, nullptr);
                vkFreeMemory(m_device->getLogicalDevice(), textureImageMemory, nullptr);

                vkDestroyDescriptorSetLayout(m_device->getLogicalDevice(), descriptorSetLayout, nullptr);

                vkDestroyBuffer(m_device->getLogicalDevice(), vertexBuffer, nullptr);
                vkFreeMemory(m_device->getLogicalDevice(), vertexBufferMemory, nullptr);
                vkDestroyBuffer(m_device->getLogicalDevice(), indexBuffer, nullptr);
                vkFreeMemory(m_device->getLogicalDevice(), indexBufferMemory, nullptr);

                for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                    vkDestroySemaphore(m_device->getLogicalDevice(), imageAvailableSemaphores[i], nullptr);
                    vkDestroySemaphore(m_device->getLogicalDevice(), renderFinishedSemaphores[i], nullptr);
                    vkDestroyFence(m_device->getLogicalDevice(), inFlightFences[i], nullptr); 
                }

                vkDestroyCommandPool(m_device->getLogicalDevice(), commandPool, nullptr);

                vkDestroyDevice(m_device->getLogicalDevice(), nullptr);

                if (enableValidationLayers) {
                    m_debug->destroy(m_instance->getInstance(), nullptr);
                }

                vkDestroySurfaceKHR(m_instance->getInstance(), m_surface->getSurface(), nullptr);
                vkDestroyInstance(m_instance->getInstance(), nullptr);

                glfwDestroyWindow(window);

                glfwTerminate();
            }

            void createSurface() {
                m_surface = new VulkanSurface(window, m_instance->getInstance());
            }

            void recreateSwapChain() {
                int width = 0, height = 0;
                while (width == 0 || height == 0) {
                    glfwGetFramebufferSize(window, &width, &height);
                    glfwWaitEvents();
                }

                vkDeviceWaitIdle(m_device->getLogicalDevice());

                cleanupSwapChain();

                m_swapChain->create(window, *m_device, m_surface->getSurface());

                createRenderPass();
                createGraphicsPipeline();
                createColorResources();
                createDepthResources();

                createFramebuffers();

                createUniformBuffers();
                createDescriptorPool();
                createDescriptorSets();
                createCommandBuffers();
            }

            void cleanupSwapChain() {
                vkDestroyImageView(m_device->getLogicalDevice(), colorImageView, nullptr);
                vkDestroyImage(m_device->getLogicalDevice(), colorImage, nullptr);
                vkFreeMemory(m_device->getLogicalDevice(), colorImageMemory, nullptr);

                vkDestroyImageView(m_device->getLogicalDevice(), depthImageView, nullptr);
                vkDestroyImage(m_device->getLogicalDevice(), depthImage, nullptr);
                vkFreeMemory(m_device->getLogicalDevice(), depthImageMemory, nullptr);

                for (auto framebuffer : m_swapChain->getFramebuffers()) {
                    vkDestroyFramebuffer(m_device->getLogicalDevice(), framebuffer, nullptr);
                }

                vkFreeCommandBuffers(m_device->getLogicalDevice(), commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

                vkDestroyPipeline(m_device->getLogicalDevice(), graphicsPipeline, nullptr);
                vkDestroyPipelineLayout(m_device->getLogicalDevice(), pipelineLayout, nullptr);
                vkDestroyRenderPass(m_device->getLogicalDevice(), m_renderPass->getRenderPass(), nullptr);

                for (auto imageView : m_swapChain->getImagesViews()) {
                    vkDestroyImageView(m_device->getLogicalDevice(), imageView, nullptr);
                }

                vkDestroySwapchainKHR(m_device->getLogicalDevice(), m_swapChain->getSwapChain(), nullptr);

                for (size_t i = 0; i < m_swapChain->getImages().size(); i++) {
                    vkDestroyBuffer(m_device->getLogicalDevice(), uniformBuffers[i], nullptr);
                    vkFreeMemory(m_device->getLogicalDevice(), uniformBuffersMemory[i], nullptr);
                }

                vkDestroyDescriptorPool(m_device->getLogicalDevice(), descriptorPool, nullptr);
            }

            void  createInstance() {
                m_instance = new VulkanInstance(enableValidationLayers, 
                        validationLayers, *m_debug);
            }

            void  createDebug() {
                m_debug = new VulkanDebug(m_instance->getInstance(), 
                        enableValidationLayers);
            }

            void  createDevice() {
                m_device = new VulkanDevice(m_instance->getInstance(), m_surface->getSurface(), 
                        deviceExtensions,
                        enableValidationLayers, validationLayers);
            }

            void  createSwapChain() {
                m_swapChain = new VulkanSwapChain(window, *m_device, m_surface->getSurface());
            }

            void createRenderPass() {
                m_renderPass = new VulkanRenderPass(m_swapChain, m_device);
            }

            void createGraphicsPipeline() {
                auto vertShaderCode = readFile("./src/shaders/vert.spv");
                auto fragShaderCode = readFile("./src/shaders/frag.spv");

                auto vertShaderModule = createShaderModule(vertShaderCode);
                auto fragShaderModule = createShaderModule(fragShaderCode);

                VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
                vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
                vertShaderStageInfo.module = vertShaderModule;
                vertShaderStageInfo.pName = "main";

                VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
                fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                fragShaderStageInfo.module = fragShaderModule;
                fragShaderStageInfo.pName = "main";

                VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

                VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
                vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

                auto bindingDescription = Vertex::getBindingDescription();
                auto attributeDescriptions = Vertex::getAttributeDescriptions();

                vertexInputInfo.vertexBindingDescriptionCount = 1;
                vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
                vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
                vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

                VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
                inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                inputAssembly.primitiveRestartEnable = VK_FALSE;

                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = (float) m_swapChain->getExtent().width;
                viewport.height = (float) m_swapChain->getExtent().height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                VkRect2D scissor{};
                scissor.offset = {0, 0};
                scissor.extent = m_swapChain->getExtent();

                VkPipelineViewportStateCreateInfo viewportState{};
                viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                viewportState.viewportCount = 1;
                viewportState.pViewports = &viewport;
                viewportState.scissorCount = 1;
                viewportState.pScissors = &scissor;

                VkPipelineRasterizationStateCreateInfo rasterizer{};
                rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                rasterizer.depthClampEnable = VK_FALSE;
                rasterizer.rasterizerDiscardEnable = VK_FALSE;
                rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
                rasterizer.lineWidth = 1.0f;
                rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
                rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                rasterizer.depthBiasEnable = VK_FALSE;

                VkPipelineMultisampleStateCreateInfo multisampling{};
                multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                multisampling.sampleShadingEnable = VK_TRUE;
                multisampling.minSampleShading = 0.2f;
                multisampling.rasterizationSamples = m_device->getMsaaSamples();

                VkPipelineDepthStencilStateCreateInfo depthStencil{};
                depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthStencil.depthTestEnable = VK_TRUE;
                depthStencil.depthWriteEnable = VK_TRUE;
                depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
                depthStencil.depthBoundsTestEnable = VK_FALSE;
                depthStencil.stencilTestEnable = VK_FALSE;

                VkPipelineColorBlendAttachmentState colorBlendAttachment{};
                colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                colorBlendAttachment.blendEnable = VK_FALSE;

                VkPipelineColorBlendStateCreateInfo colorBlending{};
                colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colorBlending.logicOpEnable = VK_FALSE;
                colorBlending.logicOp = VK_LOGIC_OP_COPY;
                colorBlending.attachmentCount = 1;
                colorBlending.pAttachments = &colorBlendAttachment;
                colorBlending.blendConstants[0] = 0.0f;
                colorBlending.blendConstants[1] = 0.0f;
                colorBlending.blendConstants[2] = 0.0f;
                colorBlending.blendConstants[3] = 0.0f;

                VkDynamicState dynamicStates[] = {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_LINE_WIDTH
                };

                VkPipelineDynamicStateCreateInfo dynamicState{};
                dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                dynamicState.dynamicStateCount = 2;
                dynamicState.pDynamicStates = dynamicStates;

                VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
                pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutInfo.setLayoutCount = 1;
                pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

                if (vkCreatePipelineLayout(m_device->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
                    throw std::runtime_error("Pipeline layout creation failed!");
                }

                VkGraphicsPipelineCreateInfo pipelineInfo{};
                pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                pipelineInfo.stageCount = 2;
                pipelineInfo.pStages = shaderStages;
                pipelineInfo.pVertexInputState = &vertexInputInfo;
                pipelineInfo.pInputAssemblyState = &inputAssembly;
                pipelineInfo.pViewportState = &viewportState;
                pipelineInfo.pRasterizationState = &rasterizer;
                pipelineInfo.pMultisampleState = &multisampling;
                pipelineInfo.pDepthStencilState = &depthStencil;
                pipelineInfo.pColorBlendState = &colorBlending;
                pipelineInfo.layout = pipelineLayout;
                pipelineInfo.renderPass = m_renderPass->getRenderPass();
                pipelineInfo.subpass = 0;
                pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

                if (vkCreateGraphicsPipelines(m_device->getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
                    throw std::runtime_error("Graphics pipeline creation failed!");
                }

                vkDestroyShaderModule(m_device->getLogicalDevice(), vertShaderModule, nullptr);
                vkDestroyShaderModule(m_device->getLogicalDevice(), fragShaderModule, nullptr);
            }

            void createFramebuffers() {
                const std::vector<VkImageView> attachments {
                    colorImageView,
                    depthImageView
                };

                m_swapChain->createFramebuffers(m_device->getLogicalDevice(), 
                        m_renderPass->getRenderPass(), attachments);
            }

            void createCommandPool() {
                QueueFamilyIndices queueFamilyIndicies = m_device->getQueueFamilyIndices();

                VkCommandPoolCreateInfo poolInfo{};
                poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                poolInfo.queueFamilyIndex = queueFamilyIndicies.graphicsFamily.value();
                poolInfo.flags = 0; 

                if (vkCreateCommandPool(m_device->getLogicalDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
                    throw std::runtime_error("One command pool creation failed!");
                }
            }

            void createDepthResources() {
                VkFormat depthFormat = m_device->findDepthFormat(); 

                createImage(m_swapChain->getExtent().width, m_swapChain->getExtent().height, 1, m_device->getMsaaSamples(), depthFormat, VK_IMAGE_TILING_OPTIMAL, 
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
                depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
            }

            void createTextureImage() {
                int texWidth, texHeight, texChannels;
                stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

                VkDeviceSize imageSize = texWidth * texHeight * 4;

                mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

                if (!pixels) {
                    throw std::runtime_error("Texture image loading failed!");
                }

                VkBuffer stagingBuffer;
                VkDeviceMemory stagingBufferMemory;

                createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

                void* data;
                vkMapMemory(m_device->getLogicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
                memcpy(data, pixels, static_cast<size_t>(imageSize));
                vkUnmapMemory(m_device->getLogicalDevice(), stagingBufferMemory);

                stbi_image_free(pixels);

                createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

                transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
                copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

                /* transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels); */

                vkDestroyBuffer(m_device->getLogicalDevice(), stagingBuffer, nullptr);
                vkFreeMemory(m_device->getLogicalDevice(), stagingBufferMemory, nullptr);

                generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
            }

            void createTextureImageView() {
                textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
            }

            void createTextureSampler() {
                VkSamplerCreateInfo samplerInfo{};

                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerInfo.magFilter = VK_FILTER_LINEAR;
                samplerInfo.minFilter = VK_FILTER_LINEAR;
                samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerInfo.anisotropyEnable = VK_TRUE;
                samplerInfo.maxAnisotropy = 16.0f;
                samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
                samplerInfo.unnormalizedCoordinates = VK_FALSE;
                samplerInfo.compareEnable = VK_FALSE;
                samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
                samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                samplerInfo.minLod = 0;
                /* samplerInfo.minLod = static_cast<float>(mipLevels / 2); */
                samplerInfo.maxLod = static_cast<float>(mipLevels);
                samplerInfo.mipLodBias = 0.0f;

                if (vkCreateSampler(m_device->getLogicalDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
                    throw std::runtime_error("Sampler creation failed!");
                }
            }

            void createColorResources() {
                VkFormat colorFormat = m_swapChain->getImageFormat();

                createImage(m_swapChain->getExtent().width, m_swapChain->getExtent().height, 1, m_device->getMsaaSamples(), colorFormat, 
                        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
                colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
            }

            void loadModel() {
                tinyobj::attrib_t attrib;
                std::vector<tinyobj::shape_t> shapes;
                std::vector<tinyobj::material_t> materials;
                std::string warn, err;

                if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
                    throw std::runtime_error(warn + err);
                }

                std::unordered_map<Vertex, uint32_t> uniqueVertices{};

                for (const auto& shape : shapes) {
                    for (const auto& index : shape.mesh.indices) {
                        Vertex vertex{};

                        vertex.pos = {
                            attrib.vertices[3 * index.vertex_index + 0],
                            attrib.vertices[3 * index.vertex_index + 1],
                            attrib.vertices[3 * index.vertex_index + 2]
                        };

                        vertex.texCoord = {
                            attrib.texcoords[2 * index.texcoord_index + 0],
                            1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
                        };

                        vertex.color = {1.0f, 1.0f, 1.0f};

                        if (uniqueVertices.count(vertex) == 0) {
                            uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                            vertices.push_back(vertex);
                        }

                        indices.push_back(uniqueVertices[vertex]);
                    }
                }
            }

            VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
                VkImageViewCreateInfo viewInfo{};
                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = image;
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format = format;
                viewInfo.subresourceRange.aspectMask = aspectFlags;
                viewInfo.subresourceRange.baseMipLevel = 0;
                viewInfo.subresourceRange.levelCount = mipLevels;
                viewInfo.subresourceRange.baseArrayLayer = 0;
                viewInfo.subresourceRange.layerCount = 1;

                VkImageView imageView;
                if (vkCreateImageView(m_device->getLogicalDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
                    throw std::runtime_error("Image view creation failed!");
                }

                return imageView;
            }

            void createVertexBuffer() {
                VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
                VkBuffer stagingBuffer;
                VkDeviceMemory stagingBufferMemory;

                createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

                void* data;
                vkMapMemory(m_device->getLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
                memcpy(data, vertices.data(), (size_t) bufferSize);
                vkUnmapMemory(m_device->getLogicalDevice(), stagingBufferMemory);

                createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

                copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

                vkDestroyBuffer(m_device->getLogicalDevice(), stagingBuffer, nullptr);
                vkFreeMemory(m_device->getLogicalDevice(), stagingBufferMemory, nullptr);
            }

            void createIndexBuffer() {
                VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
                VkBuffer stagingBuffer;
                VkDeviceMemory stagingBufferMemory;

                createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

                void* data;
                vkMapMemory(m_device->getLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
                memcpy(data, indices.data(), (size_t) bufferSize);
                vkUnmapMemory(m_device->getLogicalDevice(), stagingBufferMemory);

                createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

                copyBuffer(stagingBuffer, indexBuffer, bufferSize);

                vkDestroyBuffer(m_device->getLogicalDevice(), stagingBuffer, nullptr);
                vkFreeMemory(m_device->getLogicalDevice(), stagingBufferMemory, nullptr);
            }

            void createCommandBuffers() {
                commandBuffers.resize(m_swapChain->getFramebuffers().size());

                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.commandPool = commandPool;
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

                if (vkAllocateCommandBuffers(m_device->getLogicalDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
                    throw std::runtime_error("Command buffers allocation failed!");
                }

                std::array<VkClearValue, 2> clearValues{};
                clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
                clearValues[1].depthStencil = {1.0f, 0};

                for (size_t i = 0; i < commandBuffers.size(); i++) {
                    VkCommandBufferBeginInfo beginInfo{};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = 0;
                    beginInfo.pInheritanceInfo = nullptr;

                    if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
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

                    vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

                    VkBuffer vertexBuffers[] = {vertexBuffer};
                    VkDeviceSize offsets[] = {0};
                    vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
                    vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                    vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

                    vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
                    vkCmdEndRenderPass(commandBuffers[i]);

                    if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
                        throw std::runtime_error("Recording of a command buffer failed!");
                    }
                }
            }

            void createSyncObjects() {
                imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
                renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
                inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
                imagesInFlight.resize(m_swapChain->getImages().size(), VK_NULL_HANDLE);

                VkSemaphoreCreateInfo semaphoreInfo{};
                semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

                VkFenceCreateInfo fenceInfo{};
                fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;


                for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                    if (vkCreateSemaphore(m_device->getLogicalDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS 
                            || vkCreateSemaphore(m_device->getLogicalDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS
                            || vkCreateFence(m_device->getLogicalDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                        throw std::runtime_error("Synchronisation objects creation for a frame failed!");
                    }
                }
            }

            void createDescriptorSetLayout() {
                VkDescriptorSetLayoutBinding uboLayoutBinding{};
                uboLayoutBinding.binding = 0;
                uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uboLayoutBinding.descriptorCount = 1;
                uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                uboLayoutBinding.pImmutableSamplers = nullptr;

                VkDescriptorSetLayoutBinding samplerLayoutBinding{};
                samplerLayoutBinding.binding = 1;
                samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                samplerLayoutBinding.descriptorCount = 1;
                samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                samplerLayoutBinding.pImmutableSamplers = nullptr;

                std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

                VkDescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
                layoutInfo.pBindings = bindings.data();

                if (vkCreateDescriptorSetLayout(m_device->getLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
                    throw std::runtime_error("Set layout descriptor creation failed!");
                }
            }

            void createUniformBuffers() {
                VkDeviceSize bufferSize = sizeof(UniformBufferObject);

                uniformBuffers.resize(m_swapChain->getImages().size());
                uniformBuffersMemory.resize(m_swapChain->getImages().size());

                for (size_t i = 0; i < m_swapChain->getImages().size(); i++) {
                    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
                }
            }

            void createDescriptorPool() {
                std::array<VkDescriptorPoolSize, 2> poolSizes{};

                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = static_cast<uint32_t>(m_swapChain->getImages().size());
                poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[1].descriptorCount = static_cast<uint32_t>(m_swapChain->getImages().size());


                VkDescriptorPoolCreateInfo poolInfo{};
                poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
                poolInfo.pPoolSizes = poolSizes.data();
                poolInfo.maxSets = static_cast<uint32_t>(m_swapChain->getImages().size());
                poolInfo.flags = 0;

                if (vkCreateDescriptorPool(m_device->getLogicalDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
                    throw std::runtime_error("Descriptor pool creation failed!");
                }
            }

            void createDescriptorSets() {
                std::vector<VkDescriptorSetLayout> layouts(m_swapChain->getImages().size(), descriptorSetLayout);
                VkDescriptorSetAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool = descriptorPool;
                allocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapChain->getImages().size());
                allocInfo.pSetLayouts = layouts.data();

                descriptorSets.resize(m_swapChain->getImages().size());

                if (vkAllocateDescriptorSets(m_device->getLogicalDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
                    throw std::runtime_error("Descriptor set allocation failed!");
                }

                for (size_t i = 0; i < m_swapChain->getImages().size(); i++) {
                    VkDescriptorBufferInfo bufferInfo{};
                    bufferInfo.buffer = uniformBuffers[i];
                    bufferInfo.offset = 0;
                    bufferInfo.range = sizeof(UniformBufferObject);

                    VkDescriptorImageInfo imageInfo{};
                    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfo.imageView = textureImageView;
                    imageInfo.sampler = textureSampler;

                    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

                    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrites[0].dstSet = descriptorSets[i];
                    descriptorWrites[0].dstBinding = 0;
                    descriptorWrites[0].dstArrayElement = 0;
                    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    descriptorWrites[0].descriptorCount = 1;
                    descriptorWrites[0].pBufferInfo = &bufferInfo;

                    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrites[1].dstSet = descriptorSets[i];
                    descriptorWrites[1].dstBinding = 1;
                    descriptorWrites[1].dstArrayElement = 0;
                    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    descriptorWrites[1].descriptorCount = 1;
                    descriptorWrites[1].pImageInfo = &imageInfo;

                    vkUpdateDescriptorSets(m_device->getLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
                }
            }

            VkShaderModule createShaderModule(const std::vector<char>& code) {
                VkShaderModuleCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                createInfo.codeSize = code.size();
                createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

                VkShaderModule shaderModule;
                if (vkCreateShaderModule(m_device->getLogicalDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
                    throw std::runtime_error("Module shader creation failed!");
                }

                return shaderModule;
            }

            static std::vector<char> readFile(const std::string& filename) {
                std::ifstream file(filename, std::ios::ate | std::ios::binary);

                if (!file.is_open()) {
                    throw std::runtime_error(std::string {"File "} + filename + " opening failed!");
                }

                size_t fileSize = (size_t) file.tellg();
                std::vector<char> buffer(fileSize);

                file.seekg(0);
                file.read(buffer.data(), fileSize);

                file.close();

                return buffer;
            }

            uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
                VkPhysicalDeviceMemoryProperties memProperties;
                vkGetPhysicalDeviceMemoryProperties(m_device->getPhysicalDevice(), &memProperties);

                for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
                    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                        return i;
                    }
                }

                throw std::runtime_error("No memory type is available for the buffer!");
            }

            void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
                VkBufferCreateInfo bufferInfo{};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = size;
                bufferInfo.usage = usage;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                if (vkCreateBuffer(m_device->getLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
                    throw std::runtime_error("Buffer creation failed!");
                }

                VkMemoryRequirements memRequirements;
                vkGetBufferMemoryRequirements(m_device->getLogicalDevice(), buffer, &memRequirements);

                VkMemoryAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                allocInfo.allocationSize = memRequirements.size;
                allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

                if (vkAllocateMemory(m_device->getLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
                    throw std::runtime_error("Memory allocation failed!");
                }

                vkBindBufferMemory(m_device->getLogicalDevice(), buffer, bufferMemory, 0);
            }

            void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
                VkCommandBuffer commandBuffer = beginSingleTimeCommands();

                VkBufferCopy copyRegion{};
                copyRegion.size = size;
                vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

                endSingleTimeCommands(commandBuffer);
            }

            VkCommandBuffer beginSingleTimeCommands() {
                VkCommandBufferAllocateInfo allocInfo{};

                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocInfo.commandPool = commandPool;
                allocInfo.commandBufferCount = 1;

                VkCommandBuffer commandBuffer;
                vkAllocateCommandBuffers(m_device->getLogicalDevice(), &allocInfo, &commandBuffer);

                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

                vkBeginCommandBuffer(commandBuffer, &beginInfo);

                return commandBuffer;
            }

            void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
                vkEndCommandBuffer(commandBuffer);

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandBuffer;

                vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
                vkQueueWaitIdle(m_device->getGraphicsQueue());

                vkFreeCommandBuffers(m_device->getLogicalDevice(), commandPool, 1, &commandBuffer);
            }

            void updateCamera(uint32_t currentImage) {
                CoordinatesUniformBufferObject ubo{};

                ubo.model = glm::mat4(1.0f);
                ubo.model = glm::rotate(ubo.model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                ubo.model = glm::rotate(ubo.model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

                ubo.view = cam.getViewMatrix();

                ubo.proj = glm::perspective(glm::radians(cam.getZoom()), m_swapChain->getExtent().width / (float) m_swapChain->getExtent().height, 0.1f,  100.0f);
                ubo.proj[1][1] *= -1;

                void* data;
                vkMapMemory(m_device->getLogicalDevice(), uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
                memcpy(data, &ubo, sizeof(ubo));
                vkUnmapMemory(m_device->getLogicalDevice(), uniformBuffersMemory[currentImage]);
            }

            void updateUniformBuffer(uint32_t currentImage) {
                UniformBufferObject ubo{};
                ubo.model = glm::mat4(1.0f);
                ubo.view = glm::mat4(1.0f);
                ubo.view = glm::translate(ubo.view, glm::vec3(2.0f, 2.0f, 2.0f));
                ubo.proj = glm::perspective(glm::radians(45.0f), m_swapChain->getExtent().width / (float) m_swapChain->getExtent().height, 0.1f,  10.0f);
                ubo.proj[1][1] *= -1;

                void* data;
                vkMapMemory(m_device->getLogicalDevice(), uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
                memcpy(data, &ubo, sizeof(ubo));
                vkUnmapMemory(m_device->getLogicalDevice(), uniformBuffersMemory[currentImage]);
            }

            void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory &imageMemory) {
                VkImageCreateInfo imageInfo{};
                imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                imageInfo.extent.width = width;
                imageInfo.extent.height = height;
                imageInfo.extent.depth = 1;
                imageInfo.mipLevels = mipLevels;
                imageInfo.samples = numSamples;
                imageInfo.arrayLayers = 1;
                imageInfo.format = format;
                imageInfo.tiling = tiling;
                imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo.usage = usage;
                imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                if (vkCreateImage(m_device->getLogicalDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
                    throw std::runtime_error("Image creation failed!");
                }

                VkMemoryRequirements memRequirements;
                vkGetImageMemoryRequirements(m_device->getLogicalDevice(), image, &memRequirements);

                VkMemoryAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                allocInfo.allocationSize = memRequirements.size;
                allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                if (vkAllocateMemory(m_device->getLogicalDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
                    throw std::runtime_error("Memory allocation for an image failed!");
                }

                vkBindImageMemory(m_device->getLogicalDevice(), image, imageMemory, 0);
            }

            void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
                VkCommandBuffer commandBuffer = beginSingleTimeCommands();

                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = oldLayout;
                barrier.newLayout = newLayout;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = image;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = mipLevels;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;

                VkPipelineStageFlags sourceStage;
                VkPipelineStageFlags destinationStage;

                if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                    barrier.srcAccessMask = 0;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                } else {
                    throw std::runtime_error("Organisation of a transition not supported!");
                }

                vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                endSingleTimeCommands(commandBuffer);
            }

            void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
                VkCommandBuffer commandBuffer = beginSingleTimeCommands();

                VkBufferImageCopy region{};
                region.bufferOffset = 0;
                region.bufferRowLength = 0;
                region.bufferImageHeight = 0;

                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = 0;
                region.imageSubresource.baseArrayLayer = 0;
                region.imageSubresource.layerCount = 1;

                region.imageOffset = {0, 0, 0};
                region.imageExtent = {
                    width,
                    height,
                    1
                };

                vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

                endSingleTimeCommands(commandBuffer);
            }

            bool hasStencilComponent(VkFormat format) {
                return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
            }

            void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
                VkFormatProperties formatProperties;
                vkGetPhysicalDeviceFormatProperties(m_device->getPhysicalDevice(), imageFormat, &formatProperties);

                if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
                    throw std::runtime_error("Image texture format does not support linear filtering");
                }

                VkCommandBuffer commandBuffer = beginSingleTimeCommands();

                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.image = image;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.subresourceRange.levelCount = 1;

                int32_t mipWidth = texWidth;
                int32_t mipHeight = texHeight;

                for (uint32_t i = 1; i < mipLevels; i++) {
                    barrier.subresourceRange.baseMipLevel = i - 1;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                    vkCmdPipelineBarrier(commandBuffer, 
                            VK_PIPELINE_STAGE_TRANSFER_BIT, 
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &barrier);

                    VkImageBlit blit{};
                    blit.srcOffsets[0] = {0, 0, 0};
                    blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
                    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.srcSubresource.mipLevel = i - 1;
                    blit.srcSubresource.baseArrayLayer = 0;
                    blit.srcSubresource.layerCount = 1;
                    blit.dstOffsets[0] = {0, 0, 0};
                    blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
                    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.dstSubresource.mipLevel = i;
                    blit.dstSubresource.baseArrayLayer = 0;
                    blit.dstSubresource.layerCount = 1;

                    vkCmdBlitImage(commandBuffer, 
                            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, 
                            VK_FILTER_LINEAR);

                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    vkCmdPipelineBarrier(commandBuffer, 
                            VK_PIPELINE_STAGE_TRANSFER_BIT, 
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &barrier);

                    if (mipWidth > 1) mipWidth /= 2;
                    if (mipHeight > 1) mipHeight /= 2;
                }

                barrier.subresourceRange.baseMipLevel = mipLevels - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer, 
                        VK_PIPELINE_STAGE_TRANSFER_BIT, 
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &barrier);

                endSingleTimeCommands(commandBuffer);
            }

            static void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
                if (key == GLFW_KEY_ESCAPE) {
                    if (action == GLFW_PRESS)
                        glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
                if (key == GLFW_KEY_M) {
                    if (action == GLFW_PRESS) {
                        captureMouse = !captureMouse;
                        if (captureMouse) {
                            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                        } else {
                            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                        }
                    }
                }
            }

            static void scroll_callback(GLFWwindow* window, double xOffset, double yOffset) {
                cam.processZoom(yOffset);
            }

            static void mouse_callback(GLFWwindow* window, double xPos, double yPos) {
                cam.processMouse(xPos, yPos, captureMouse);
            }

            void processKeyboardInput() {

                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                    cam.processMovement(VulkanLearning::UPWARD, fpsCounter.getDeltaTime());
                } else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                    cam.processMovement(VulkanLearning::FORWARD, fpsCounter.getDeltaTime());
                }

                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                    cam.processMovement(VulkanLearning::DOWNWARD, fpsCounter.getDeltaTime());
                } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                    cam.processMovement(VulkanLearning::BACKWARD, fpsCounter.getDeltaTime());
                }

                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                    cam.processMovement(VulkanLearning::LEFT, fpsCounter.getDeltaTime());
                }

                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                    cam.processMovement(VulkanLearning::RIGHT, fpsCounter.getDeltaTime());
                }
            }
    };

}

int main() {
    VulkanLearning::HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
