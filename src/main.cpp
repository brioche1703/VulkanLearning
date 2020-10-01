#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>

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

#include "../include/VulkanLearning/base/Vertex.hpp"

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
#include "../include/VulkanLearning/base/VulkanGraphicsPipeline.hpp"
#include "../include/VulkanLearning/misc/model/ModelObj.hpp"

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

            VulkanGraphicsPipeline* m_graphicsPipeline;

            VulkanCommandPool* m_commandPool;

            ModelObj* m_model;

            VulkanBuffer* m_vertexBuffer;
            VulkanBuffer* m_indexBuffer;

            std::vector<VulkanBuffer*> m_coordinateSystemUniformBuffers;

            VulkanCommandBuffers* m_commandBuffers;

            VulkanSyncObjects* m_syncObjects;

            size_t currentFrame = 0;
            bool framebufferResized = false;

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
                createTexture();

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

                m_texture->cleanup();

                m_descriptorSetLayout->cleanup();

                m_vertexBuffer->cleanup();
                m_indexBuffer->cleanup();

                m_syncObjects->cleanup();

                m_commandPool->cleanup();

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

            void cleanupSwapChain() {
                m_colorImageResource->cleanup();
                m_depthImageResource->cleanup();

                m_swapChain->cleanFramebuffers();

                m_commandBuffers->cleanup();

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
                m_swapChain = new VulkanSwapChain(m_window, m_device, m_surface);
            }

            void createRenderPass() {
                m_renderPass = new VulkanRenderPass(m_swapChain, m_device);
            }

            void createGraphicsPipeline() {
                m_graphicsPipeline = new VulkanGraphicsPipeline(m_device,
                        m_swapChain, m_renderPass, m_descriptorSetLayout);
            }

            void createFramebuffers() {
                const std::vector<VkImageView> attachments {
                    m_colorImageResource->getImageView(),
                        m_depthImageResource->getImageView()
                };

                m_swapChain->createFramebuffers(m_renderPass->getRenderPass(), 
                        attachments);
            }

            void createCommandPool() {
                m_commandPool = new VulkanCommandPool(m_device);
            }

            void createTexture() {
                m_texture = new VulkanTexture(TEXTURE_PATH, m_device, m_swapChain,
                        m_commandPool);
                m_texture->create();
                m_texture->createImageView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
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

            void createVertexBuffer() {
                m_vertexBuffer = new VulkanBuffer(m_device, m_commandPool);
                m_vertexBuffer->createWithStagingBuffer(m_model->getVerticies(), 
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            }

            void createIndexBuffer() {
                m_indexBuffer = new VulkanBuffer(m_device, m_commandPool);
                m_indexBuffer->createWithStagingBuffer(m_model->getIndicies(), 
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
                m_commandBuffers = new VulkanCommandBuffers(m_device, 
                        m_swapChain, m_commandPool, m_renderPass, m_vertexBuffer,
                        m_indexBuffer, static_cast<uint32_t>(m_model->getIndicies().size()),
                        m_graphicsPipeline->getGraphicsPipeline(),
                        m_graphicsPipeline->getPipelineLayout(), 
                        m_descriptorSets);
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

            void updateCamera(uint32_t currentImage) {
                CoordinatesSystemUniformBufferObject ubo{};

                ubo.model = glm::mat4(1.0f);
                ubo.model = glm::rotate(ubo.model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                ubo.model = glm::rotate(ubo.model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

                ubo.view = m_camera->getViewMatrix();

                ubo.proj = glm::perspective(glm::radians(m_camera->getZoom()), 
                        m_swapChain->getExtent().width / (float) m_swapChain->getExtent().height, 
                        0.1f,  100.0f);

                ubo.proj[1][1] *= -1;

                void* data;
                vkMapMemory(m_device->getLogicalDevice(), 
                        m_coordinateSystemUniformBuffers[currentImage]->getBufferMemory(), 
                        0, sizeof(ubo), 0, &data);
                memcpy(data, &ubo, sizeof(ubo));
                vkUnmapMemory(m_device->getLogicalDevice(), 
                        m_coordinateSystemUniformBuffers[currentImage]->getBufferMemory());
            }

            void loadModel() {
                m_model = new ModelObj(MODEL_PATH);
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
