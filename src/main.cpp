#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
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
#include <functional>

#include "../include/VulkanLearning/camera/camera.hpp"
#include "../include/VulkanLearning/misc/FpsCounter.hpp"
#include "../include/VulkanLearning/misc/VulkanDebug.hpp"
#include "../include/VulkanLearning/base/VulkanDevice.hpp"
#include "../include/VulkanLearning/base/VulkanInstance.hpp"
#include "../include/VulkanLearning/base/VulkanSwapChain.hpp"
#include "../include/VulkanLearning/base/VulkanSurface.hpp"
#include "../include/VulkanLearning/base/VulkanRenderPass.hpp"
#include "../include/VulkanLearning/misc/Inputs.hpp"
#include "../include/VulkanLearning/window/Window.hpp"
#include "../include/VulkanLearning/base/VulkanDescriptorSetLayout.hpp"
#include "../include/VulkanLearning/base/VulkanCommandPool.hpp"
#include "../include/VulkanLearning/base/VulkanShaderModule.hpp"
#include "../include/VulkanLearning/base/VulkanBuffer.hpp"
#include "../include/VulkanLearning/base/VulkanDescriptorPool.hpp"
#include "../include/VulkanLearning/base/VulkanDescriptorSets.hpp"
#include "../include/VulkanLearning/base/VulkanCommandBuffers.hpp"
#include "../include/VulkanLearning/base/VulkanSyncObjects.hpp"
#include "../include/VulkanLearning/base/VulkanImageResource.hpp"
#include "../include/VulkanLearning/base/VulkanCommandBuffer.hpp"
#include "../include/VulkanLearning/base/VulkanTexture.hpp"

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

struct CoordinatesSystemUniformBufferObject {
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
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_MESA_overlay",
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    static bool captureMouse = true;

    class HelloTriangleApplication {
        public:
            void run() {
                initWindow();
                initCore();
                initVulkan();
                mainLoop();
                cleanup();
            }

        private:
            Window* m_window;
            Camera* m_camera;
            FpsCounter* m_fpsCounter;
            Inputs* m_input;

            VulkanInstance *m_instance;
            VulkanDebug* m_debug = new VulkanDebug();
            VulkanDevice* m_device;
            VulkanSurface* m_surface;

            VulkanSwapChain *m_swapChain;

            VulkanRenderPass* m_renderPass;

            VulkanDescriptorSetLayout* m_descriptorSetLayout;
            VulkanDescriptorPool* m_descriptorPool;

            VulkanDescriptorSets* m_descriptorSets;

            VkPipelineLayout pipelineLayout;

            VkPipeline graphicsPipeline;

            VulkanCommandPool* m_commandPool;

            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            VulkanBuffer* m_vertexBuffer;
            VulkanBuffer* m_indexBuffer;

            std::vector<VulkanBuffer*> m_coordinateSystemUniformBuffers;

            VulkanCommandBuffers* m_commandBuffers;

            VulkanSyncObjects* m_syncObjects;

            size_t currentFrame = 0;

            bool framebufferResized = false;

            uint32_t mipLevels;

            VulkanTexture* m_texture;

            VulkanImageResource* m_colorImageResource;
            VulkanImageResource* m_depthImageResource;

            void initWindow() {
                m_window = new Window("Vulkan", WIDTH, HEIGHT);
                m_window->init();
            }

            void initCore() {
                m_camera = new Camera(glm::vec3(0.0f, 0.0f, 5.0f));
                m_fpsCounter = new FpsCounter();
                m_input = new Inputs(m_window->getWindow(), m_camera, m_fpsCounter);

                glfwSetKeyCallback(m_window->getWindow() , m_input->keyboard_callback);
                glfwSetScrollCallback(m_window->getWindow() , m_input->scroll_callback);
                glfwSetCursorPosCallback(m_window->getWindow() , m_input->mouse_callback);

                glfwSetInputMode(m_window->getWindow() , GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
                createCoordinateSystemUniformBuffers();
                createDescriptorPool();
                createDescriptorSets();
                createCommandBuffers();
                createSyncObjects();
            }

            void mainLoop() {
                while (!glfwWindowShouldClose(m_window->getWindow() )) {
                    glfwPollEvents();
                    m_input->processKeyboardInput();
                    m_fpsCounter->update();
                    drawFrame();
                }

                vkDeviceWaitIdle(m_device->getLogicalDevice());
            }

            void drawFrame() {
                vkWaitForFences(m_device->getLogicalDevice(), 1, &m_syncObjects->getInFlightFences()[currentFrame], VK_TRUE, UINT64_MAX);

                uint32_t imageIndex;

                VkResult result = vkAcquireNextImageKHR(m_device->getLogicalDevice(), m_swapChain->getSwapChain(), UINT64_MAX, m_syncObjects->getImageAvailableSemaphores()[currentFrame], VK_NULL_HANDLE, &imageIndex);

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
                submitInfo.pCommandBuffers = m_commandBuffers->getCommandBufferPointer(imageIndex);

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

            void cleanup() {
                cleanupSwapChain();

                vkDestroySampler(m_device->getLogicalDevice(), m_texture->getSampler(), nullptr);
                vkDestroyImageView(m_device->getLogicalDevice(), m_texture->getImageView(), nullptr);

                vkDestroyImage(m_device->getLogicalDevice(), m_texture->getImage(), nullptr);
                vkFreeMemory(m_device->getLogicalDevice(), m_texture->getDeviceMemory(), nullptr);

                vkDestroyDescriptorSetLayout(m_device->getLogicalDevice(), m_descriptorSetLayout->getDescriptorSetLayout(), nullptr);

                vkDestroyBuffer(m_device->getLogicalDevice(), m_vertexBuffer->getBuffer(), nullptr);
                vkFreeMemory(m_device->getLogicalDevice(), m_vertexBuffer->getBufferMemory(), nullptr);
                vkDestroyBuffer(m_device->getLogicalDevice(), m_indexBuffer->getBuffer(), nullptr);
                vkFreeMemory(m_device->getLogicalDevice(), m_indexBuffer->getBufferMemory(), nullptr);

                for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                    vkDestroySemaphore(m_device->getLogicalDevice(), m_syncObjects->getImageAvailableSemaphores()[i], nullptr);
                    vkDestroySemaphore(m_device->getLogicalDevice(), m_syncObjects->getRenderFinishedSemaphores()[i], nullptr);
                    vkDestroyFence(m_device->getLogicalDevice(), m_syncObjects->getInFlightFences()[i], nullptr); 
                }

                vkDestroyCommandPool(m_device->getLogicalDevice(), m_commandPool->getCommandPool(), nullptr);

                vkDestroyDevice(m_device->getLogicalDevice(), nullptr);

                if (enableValidationLayers) {
                    m_debug->destroy(m_instance->getInstance(), nullptr);
                }

                vkDestroySurfaceKHR(m_instance->getInstance(), m_surface->getSurface(), nullptr);
                vkDestroyInstance(m_instance->getInstance(), nullptr);

                glfwDestroyWindow(m_window->getWindow() );

                glfwTerminate();
            }

            void createSurface() {
                m_surface = new VulkanSurface(m_window->getWindow() , m_instance->getInstance());
            }

            void recreateSwapChain() {
                int width = 0, height = 0;
                while (width == 0 || height == 0) {
                    glfwGetFramebufferSize(m_window->getWindow() , &width, &height);
                    glfwWaitEvents();
                }

                vkDeviceWaitIdle(m_device->getLogicalDevice());

                cleanupSwapChain();

                m_swapChain->create(m_window->getWindow() , *m_device, m_surface->getSurface());

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

            void cleanupSwapChain() {
                vkDestroyImageView(m_device->getLogicalDevice(), m_colorImageResource->getImageView(), nullptr);
                vkDestroyImage(m_device->getLogicalDevice(), m_colorImageResource->getImage(), nullptr);
                vkFreeMemory(m_device->getLogicalDevice(), m_colorImageResource->getDeviceMemory(), nullptr);

                vkDestroyImageView(m_device->getLogicalDevice(), m_depthImageResource->getImageView(), nullptr);
                vkDestroyImage(m_device->getLogicalDevice(), m_depthImageResource->getImage(), nullptr);
                vkFreeMemory(m_device->getLogicalDevice(), m_depthImageResource->getDeviceMemory(), nullptr);

                for (auto framebuffer : m_swapChain->getFramebuffers()) {
                    vkDestroyFramebuffer(m_device->getLogicalDevice(), framebuffer, nullptr);
                }

                vkFreeCommandBuffers(m_device->getLogicalDevice(), m_commandPool->getCommandPool(), static_cast<uint32_t>(m_commandBuffers->getCommandBuffers().size()), m_commandBuffers->getCommandBuffers().data());

                vkDestroyPipeline(m_device->getLogicalDevice(), graphicsPipeline, nullptr);
                vkDestroyPipelineLayout(m_device->getLogicalDevice(), pipelineLayout, nullptr);
                vkDestroyRenderPass(m_device->getLogicalDevice(), m_renderPass->getRenderPass(), nullptr);

                for (auto imageView : m_swapChain->getImagesViews()) {
                    vkDestroyImageView(m_device->getLogicalDevice(), imageView, nullptr);
                }

                vkDestroySwapchainKHR(m_device->getLogicalDevice(), m_swapChain->getSwapChain(), nullptr);

                for (size_t i = 0; i < m_swapChain->getImages().size(); i++) {
                    vkDestroyBuffer(m_device->getLogicalDevice(), m_coordinateSystemUniformBuffers[i]->getBuffer(), nullptr);
                    vkFreeMemory(m_device->getLogicalDevice(), m_coordinateSystemUniformBuffers[i]->getBufferMemory(), nullptr);
                }

                vkDestroyDescriptorPool(m_device->getLogicalDevice(), m_descriptorPool->getDescriptorPool(), nullptr);
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
                m_swapChain = new VulkanSwapChain(m_window->getWindow() , *m_device, m_surface->getSurface());
            }

            void createRenderPass() {
                m_renderPass = new VulkanRenderPass(m_swapChain, m_device);
            }

            void createGraphicsPipeline() {
                VulkanShaderModule vertShaderModule = VulkanShaderModule("./src/shaders/vert.spv", m_device);
                VulkanShaderModule fragShaderModule = VulkanShaderModule("./src/shaders/frag.spv", m_device);

                VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
                vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
                vertShaderStageInfo.module = vertShaderModule.getModule();
                vertShaderStageInfo.pName = "main";

                VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
                fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                fragShaderStageInfo.module = fragShaderModule.getModule();
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
                pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayout->getDescriptorSetLayoutPointer();

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

                vkDestroyShaderModule(m_device->getLogicalDevice(), vertShaderModule.getModule(), nullptr);
                vkDestroyShaderModule(m_device->getLogicalDevice(), fragShaderModule.getModule(), nullptr);
            }

            void createFramebuffers() {
                const std::vector<VkImageView> attachments {
                    m_colorImageResource->getImageView(),
                    m_depthImageResource->getImageView()
                };

                m_swapChain->createFramebuffers(m_device->getLogicalDevice(), 
                        m_renderPass->getRenderPass(), attachments);
            }

            void createCommandPool() {
                m_commandPool = new VulkanCommandPool(m_device);
            }

            void createTextureImage() {
                m_texture = new VulkanTexture(TEXTURE_PATH, m_device, m_swapChain,
                        m_commandPool);
                m_texture->create();
            }

            void createTextureImageView() {
                m_texture->createImageView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
            }

            void createTextureSampler() {
                m_texture->createSampler();
            }

            void createColorResources() {
                m_colorImageResource = new VulkanImageResource(m_device, 
                        m_swapChain, m_commandPool, 
                        m_swapChain->getImageFormat(),  
                        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
                        | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
                        VK_IMAGE_ASPECT_COLOR_BIT);
                m_colorImageResource->create();
            }

            void createDepthResources() {
                m_depthImageResource = new VulkanImageResource(m_device, 
                        m_swapChain, m_commandPool, 
                        m_device->findDepthFormat(), 
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                        VK_IMAGE_ASPECT_DEPTH_BIT);
                m_depthImageResource->create();
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
                m_vertexBuffer = new VulkanBuffer(m_device, m_commandPool);
                m_vertexBuffer->createWithStagingBuffer(vertices, 
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            }

            void createIndexBuffer() {
                m_indexBuffer = new VulkanBuffer(m_device, m_commandPool);
                m_indexBuffer->createWithStagingBuffer(indices, 
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            }

            void createCoordinateSystemUniformBuffers() {
                VkDeviceSize bufferSize = sizeof(CoordinatesSystemUniformBufferObject);

                m_coordinateSystemUniformBuffers.resize(m_swapChain->getImages().size());

                for (size_t i = 0; i < m_swapChain->getImages().size(); i++) {
                    m_coordinateSystemUniformBuffers[i] = new VulkanBuffer(m_device, m_commandPool);
                    m_coordinateSystemUniformBuffers[i]->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, *m_coordinateSystemUniformBuffers[i]->getBufferPointer(), *m_coordinateSystemUniformBuffers[i]->getBufferMemoryPointer());
                }
            }

            void createCommandBuffers() {
                m_commandBuffers = new VulkanCommandBuffers(m_device, m_swapChain,
                        m_commandPool, m_renderPass, m_vertexBuffer,
                        m_indexBuffer, static_cast<uint32_t>(indices.size()), graphicsPipeline,
                        pipelineLayout, m_descriptorSets);
            }

            void createSyncObjects() {
                m_syncObjects = new VulkanSyncObjects(m_device, m_swapChain, 
                        MAX_FRAMES_IN_FLIGHT);
            }

            void createDescriptorSetLayout() {
                m_descriptorSetLayout = new VulkanDescriptorSetLayout(m_device);
            }

            void createDescriptorPool() {
                m_descriptorPool = new VulkanDescriptorPool(m_device, m_swapChain);
            }

            void createDescriptorSets() {
                m_descriptorSets = new VulkanDescriptorSets(m_device, m_swapChain,
                        m_descriptorSetLayout, m_descriptorPool,
                        m_coordinateSystemUniformBuffers, sizeof(CoordinatesSystemUniformBufferObject),
                        m_texture);
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
                allocInfo.memoryTypeIndex = m_device->findMemoryType(memRequirements.memoryTypeBits, properties);

                if (vkAllocateMemory(m_device->getLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
                    throw std::runtime_error("Memory allocation failed!");
                }

                vkBindBufferMemory(m_device->getLogicalDevice(), buffer, bufferMemory, 0);
            }

            void updateCamera(uint32_t currentImage) {
                CoordinatesSystemUniformBufferObject ubo{};

                ubo.model = glm::mat4(1.0f);
                ubo.model = glm::rotate(ubo.model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                ubo.model = glm::rotate(ubo.model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

                ubo.view = m_camera->getViewMatrix();

                ubo.proj = glm::perspective(glm::radians(m_camera->getZoom()), m_swapChain->getExtent().width / (float) m_swapChain->getExtent().height, 0.1f,  100.0f);
                ubo.proj[1][1] *= -1;

                void* data;
                vkMapMemory(m_device->getLogicalDevice(), m_coordinateSystemUniformBuffers[currentImage]->getBufferMemory(), 0, sizeof(ubo), 0, &data);
                memcpy(data, &ubo, sizeof(ubo));
                vkUnmapMemory(m_device->getLogicalDevice(), m_coordinateSystemUniformBuffers[currentImage]->getBufferMemory());
            }

            void updateUniformBuffer(uint32_t currentImage) {
                CoordinatesSystemUniformBufferObject ubo{};
                ubo.model = glm::mat4(1.0f);
                ubo.view = glm::mat4(1.0f);
                ubo.view = glm::translate(ubo.view, glm::vec3(2.0f, 2.0f, 2.0f));
                ubo.proj = glm::perspective(glm::radians(45.0f), m_swapChain->getExtent().width / (float) m_swapChain->getExtent().height, 0.1f,  10.0f);
                ubo.proj[1][1] *= -1;

                void* data;
                vkMapMemory(m_device->getLogicalDevice(), m_coordinateSystemUniformBuffers[currentImage]->getBufferMemory(), 0, sizeof(ubo), 0, &data);
                memcpy(data, &ubo, sizeof(ubo));
                vkUnmapMemory(m_device->getLogicalDevice(), m_coordinateSystemUniformBuffers[currentImage]->getBufferMemory());
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
                allocInfo.memoryTypeIndex = m_device->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                if (vkAllocateMemory(m_device->getLogicalDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
                    throw std::runtime_error("Memory allocation for an image failed!");
                }

                vkBindImageMemory(m_device->getLogicalDevice(), image, imageMemory, 0);
            }

            void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
                /* VkCommandBuffer commandBuffer = beginSingleTimeCommands(); */
                VulkanCommandBuffer commandBuffer(m_device, m_commandPool);
                commandBuffer.beginSingleTimeCommands();

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

                vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                commandBuffer.endSingleTimeCommands();
            }

            void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
                VulkanCommandBuffer commandBuffer(m_device, m_commandPool);
                commandBuffer.beginSingleTimeCommands();

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

                vkCmdCopyBufferToImage(commandBuffer.getCommandBuffer(), buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

                commandBuffer.endSingleTimeCommands();
            }

            void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
                VkFormatProperties formatProperties;
                vkGetPhysicalDeviceFormatProperties(m_device->getPhysicalDevice(), imageFormat, &formatProperties);

                if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
                    throw std::runtime_error("Image texture format does not support linear filtering");
                }

                VulkanCommandBuffer commandBuffer(m_device, m_commandPool);
                commandBuffer.beginSingleTimeCommands();

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

                    vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), 
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

                    vkCmdBlitImage(commandBuffer.getCommandBuffer(), 
                            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, 
                            VK_FILTER_LINEAR);

                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), 
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

                vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), 
                        VK_PIPELINE_STAGE_TRANSFER_BIT, 
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &barrier);

                commandBuffer.endSingleTimeCommands();
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
