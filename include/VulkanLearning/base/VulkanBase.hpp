#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_SWIZZLE

#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>

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
#include <cstring>
#include <random>

#include "Vertex.hpp"
#include "Camera.hpp"
#include "Inputs.hpp"
#include "Window.hpp"
#include "UI.hpp"
#include "model/ModelObj.hpp"
#include "FpsCounter.hpp"

#include "VulkanDebug.hpp"
#include "VulkanDevice.hpp"
#include "VulkanInstance.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanSurface.hpp"
#include "VulkanRenderPass.hpp"
#include "VulkanShaderModule.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanDescriptorSetLayout.hpp"
#include "VulkanDescriptorPool.hpp"
#include "VulkanDescriptorSets.hpp"
#include "VulkanSyncObjects.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanImageResource.hpp"
#include "VulkanTexture.hpp"

struct CoordinatesSystemUniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 camPos;
};

namespace VulkanLearning {

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;


    const int MAX_FRAMES_IN_FLIGHT = 2;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    class VulkanBase {
        protected:
            Window m_window;
            Camera m_camera;
            FpsCounter m_fpsCounter;
            Inputs m_input;
            UI m_ui;

            VulkanInstance *m_instance;
            VulkanDebug* m_debug = new VulkanDebug();

            VulkanDevice m_device;
            uint32_t m_msaaSamples = 1;
            VulkanSurface m_surface;

            VulkanSwapChain m_swapChain;
            VkSubmitInfo m_submitInfo;

            std::vector<VkFramebuffer> m_framebuffers;

            VulkanRenderPass m_renderPass;

            VulkanDescriptorSetLayout m_descriptorSetLayout;
            VulkanDescriptorPool m_descriptorPool;

            ModelObj m_model;

            VulkanBuffer m_vertexBuffer;
            VulkanBuffer m_indexBuffer;

            std::vector<VulkanBuffer> m_coordinateSystemUniformBuffers;

            std::vector<VulkanCommandBuffer> m_commandBuffers;

            VulkanSyncObjects m_syncObjects;
            std::vector<VkSemaphore> m_signalSemaphores;
            std::vector<VkSemaphore> m_waitSemaphore;
            VkPipelineStageFlags m_waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            size_t m_currentFrame = 0;
            bool framebufferResized = false;

            VulkanImageResource m_colorImageResource;
            VulkanImageResource m_depthImageResource;

        public:
            VulkanBase() {}
            virtual ~VulkanBase() {
                for (auto i = 0; i < m_framebuffers.size(); i++) {
                    vkDestroyFramebuffer(m_device.getLogicalDevice(), m_framebuffers[i], nullptr);
                }

                m_syncObjects.cleanup();
                m_ui.freeResources();

                vkDestroyCommandPool(m_device.getLogicalDevice(), m_device.getCommandPool(), nullptr);

                vkDestroyDevice(m_device.getLogicalDevice(), nullptr);

                if (enableValidationLayers) {
                    m_debug->destroy(m_instance->getInstance(), nullptr);
                }

                vkDestroySurfaceKHR(m_instance->getInstance(), m_surface.getSurface(), nullptr);
                vkDestroyInstance(m_instance->getInstance(), nullptr);

                glfwDestroyWindow(m_window.getWindow() );

                glfwTerminate();
            }

            void run() {
                initWindow();
                initCore();
                initVulkan();
                createUI();
                mainLoop();
                cleanup();
            }

            static std::vector<const char*> args;


        protected:
            virtual void initWindow() {
                m_window = Window("Vulkan", WIDTH, HEIGHT);
                m_window.init();
            }

            virtual void initCore() {
                m_camera = Camera(glm::vec3(0.0f, 0.0f, 5.0f));
                
                m_fpsCounter = FpsCounter();
                m_input = Inputs(m_window.getWindow(), &m_camera, &m_fpsCounter, &m_ui);

                glfwSetKeyCallback(m_window.getWindow() , m_input.keyboard_callback);
                glfwSetScrollCallback(m_window.getWindow() , m_input.scroll_callback);
                glfwSetCursorPosCallback(m_window.getWindow() , m_input.mouse_callback);
                glfwSetMouseButtonCallback(m_window.getWindow() , m_input.mouse_button_callback);

                glfwSetInputMode(m_window.getWindow() , GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }

            virtual void initVulkan() {
                createInstance();
                createDebug();
                createSurface();
                createDevice();
                createSwapChain();
                createRenderPass();
                createColorResources();
                createDepthResources();
                createFramebuffers();
                createSyncObjects();
            }

            virtual void createUI() {
                ImGuiIO& io = ImGui::GetIO();
                io.DisplaySize = ImVec2((float)m_swapChain.getExtent().width, (float)m_swapChain.getExtent().height);
                m_ui.create(m_device, m_renderPass);
            }

            virtual void updateUI() {
                vkDeviceWaitIdle(m_device.getLogicalDevice());
                ImGuiIO& io = ImGui::GetIO();

                io.DisplaySize = ImVec2((float)m_swapChain.getExtent().width, (float)m_swapChain.getExtent().height);
                //io.DeltaTime = m_fpsCounter.getDeltaTime();

                io.MousePos = ImVec2(m_input.m_mousePos->x, m_input.m_mousePos->y);
                io.MouseDown[0] = m_input.m_mouseButtons->left;
                io.MouseDown[1] = m_input.m_mouseButtons->right;

                ImGui::NewFrame();

                ImGui::Begin("Vulkan Base");
                ImGui::SetWindowSize(ImVec2(0.0f, 0.0f));
                ImGui::SetWindowPos(ImVec2(10.0f, 10.0f));
                ImGui::TextUnformatted(m_window.getTitle().c_str());

                //ImGui::TextUnformatted(deviceProperties.deviceName);
                //ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / m_fpsCounter.getLastFrameTime()), m_fpsCounter.getLastFrameTime());

                ImGui::PushItemWidth(110.0f * m_ui.scale);
                OnUpdateUI(&m_ui);
                ImGui::PopItemWidth();

                ImGui::End();
                ImGui::Render();
                ImGui::EndFrame();

                if (m_ui.update() || m_ui.updated) {
                    createCommandBuffers();
                    m_ui.updated = false;
                }
            }

            virtual void drawUI(VkCommandBuffer commandBuffer) {
                if (m_ui.visible) {
                    m_ui.draw(commandBuffer);
                }
            }

            virtual void mainLoop() {
                while (!glfwWindowShouldClose(m_window.getWindow() )) {
                    glfwPollEvents();
                    m_input.processKeyboardInput();
                    m_fpsCounter.update();
                    updateUI();
                    drawFrame();
                }

                vkDeviceWaitIdle(m_device.getLogicalDevice());
            }

            virtual void acquireFrame(uint32_t *imageIndex) {
                vkWaitForFences(m_device.getLogicalDevice(), 1, &m_syncObjects.getInFlightFences()[m_currentFrame], VK_TRUE, UINT64_MAX);

                VkResult result = vkAcquireNextImageKHR(m_device.getLogicalDevice(), m_swapChain.getSwapChain(), UINT64_MAX, m_syncObjects.getImageAvailableSemaphores()[m_currentFrame], VK_NULL_HANDLE, imageIndex);

                if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                    recreateSwapChain();
                    return;
                } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                    throw std::runtime_error("Presentation of one image of the swap chain failed!");
                }

                if (m_syncObjects.getImagesInFlight()[*imageIndex] != VK_NULL_HANDLE) {
                    vkWaitForFences(m_device.getLogicalDevice(), 1, &m_syncObjects.getImagesInFlight()[*imageIndex], VK_TRUE, UINT64_MAX);
                }

                m_syncObjects.getImagesInFlight()[*imageIndex] = m_syncObjects.getInFlightFences()[m_currentFrame];

                m_submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                m_waitSemaphore = {m_syncObjects.getImageAvailableSemaphores()[m_currentFrame]};
                m_submitInfo.waitSemaphoreCount = 1;
                m_submitInfo.pWaitSemaphores = m_waitSemaphore.data();
                m_submitInfo.pWaitDstStageMask = &m_waitStages;
                m_submitInfo.commandBufferCount = 1;
                m_submitInfo.pCommandBuffers = m_commandBuffers[*imageIndex].getCommandBufferPointer();

                m_signalSemaphores = {m_syncObjects.getRenderFinishedSemaphores()[m_currentFrame]};
                m_submitInfo.signalSemaphoreCount = 1;
                m_submitInfo.pSignalSemaphores = m_signalSemaphores.data();

                vkResetFences(m_device.getLogicalDevice(), 1, &m_syncObjects.getInFlightFences()[m_currentFrame]);
            }

            virtual void presentFrame(uint32_t imageIndex) {
                VkPresentInfoKHR presentInfo{};
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = m_signalSemaphores.data();

                VkSwapchainKHR swapChains[] = {m_swapChain.getSwapChain()};
                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = swapChains;
                presentInfo.pImageIndices = &imageIndex;
                presentInfo.pResults = nullptr;

                VkResult result = vkQueuePresentKHR(m_device.getPresentQueue(), &presentInfo);

                if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
                    framebufferResized = false;
                    recreateSwapChain();
                } else if (result != VK_SUCCESS) {
                    throw std::runtime_error("Presentation of one image of the swap chain failed!");
                }

                m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            }

            virtual void drawFrame() {}

            virtual void cleanup() {}

            virtual void createSurface() {
                m_surface = VulkanSurface();
                m_surface.create(m_window, *m_instance);
            }

            virtual void recreateSwapChain() {
                int width = 0, height = 0;
                while (width == 0 || height == 0) {
                    glfwGetFramebufferSize(m_window.getWindow() , &width, &height);
                    glfwWaitEvents();
                }

                vkDeviceWaitIdle(m_device.getLogicalDevice());

                cleanupSwapChain();

                m_swapChain.create();

                createRenderPass();
                createColorResources();
                createDepthResources();

                createFramebuffers();

                createDescriptorPool();
                createDescriptorSets();
                createCommandBuffers();
                m_ui.resize(m_swapChain.getExtent().width, m_swapChain.getExtent().height);
            }

            virtual void cleanupSwapChain() {
                m_colorImageResource.cleanup();
                m_depthImageResource.cleanup();

                for (uint32_t i = 0; i < m_framebuffers.size(); i++) {
                    vkDestroyFramebuffer(m_device.getLogicalDevice(), m_framebuffers[i], nullptr);
                }

                vkFreeCommandBuffers(
                        m_device.getLogicalDevice(), 
                        m_device.getCommandPool(), 
                        static_cast<uint32_t>(m_commandBuffers.size()), 
                        m_commandBuffers.data()->getCommandBufferPointer());

                vkDestroyRenderPass(m_device.getLogicalDevice(), m_renderPass.getRenderPass(), nullptr);

                m_swapChain.destroyImageViews();
                vkDestroySwapchainKHR(m_device.getLogicalDevice(), m_swapChain.getSwapChain(), nullptr);

                vkDestroyDescriptorPool(
                        m_device.getLogicalDevice(), 
                        m_descriptorPool.getDescriptorPool(), 
                        nullptr);
            }

            virtual void createInstance() {
                m_instance = new VulkanInstance(
                        "Base",
                        enableValidationLayers, 
                        validationLayers, 
                        m_debug);
            }

            virtual void createDebug() {
                m_debug = new VulkanDebug(m_instance->getInstance(), 
                        enableValidationLayers);
            }

            virtual void checkAndEnableFeatures() {}

            virtual void createDevice() {
                m_device = VulkanDevice(m_msaaSamples);
                m_device.pickPhysicalDevice(
                        m_instance->getInstance(), 
                        m_surface.getSurface(), 
                        deviceExtensions);

                checkAndEnableFeatures();

                m_device.createLogicalDevice(
                        m_surface.getSurface(), 
                        enableValidationLayers, 
                        validationLayers);
            }

            virtual void createSwapChain() {
                m_swapChain = VulkanSwapChain(m_window, m_device, m_surface);
            }

            virtual void createRenderPass() {}

            virtual void createGraphicsPipeline() = 0;

            virtual void createFramebuffers() {
                if (m_device.getMsaaSamples() > 1) {
                    VkImageView attachments[3];

                    attachments[0] = m_colorImageResource.getImageView();
                    attachments[1] = m_depthImageResource.getImageView();

                    VkFramebufferCreateInfo framebufferCreateInfo = {};
                    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    framebufferCreateInfo.pNext = nullptr;
                    framebufferCreateInfo.renderPass = m_renderPass.getRenderPass();
                    framebufferCreateInfo.attachmentCount = 3;
                    framebufferCreateInfo.pAttachments = attachments;
                    framebufferCreateInfo.width = m_swapChain.getExtent().width;
                    framebufferCreateInfo.height = m_swapChain.getExtent().height;
                    framebufferCreateInfo.layers = 1;

                    m_framebuffers.resize(m_swapChain.getImagesViews().size());

                    for (size_t i = 0; i < m_framebuffers.size(); i++) {
                        attachments[2] = m_swapChain.getImagesViews()[i];
                        VK_CHECK_RESULT(vkCreateFramebuffer(
                                    m_device.getLogicalDevice(), 
                                    &framebufferCreateInfo, 
                                    nullptr, 
                                    &m_framebuffers[i]));
                    }

                } else {
                    VkImageView attachments[2];

                    attachments[1] = m_depthImageResource.getImageView();

                    VkFramebufferCreateInfo framebufferCreateInfo = {};
                    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    framebufferCreateInfo.pNext = nullptr;
                    framebufferCreateInfo.renderPass = m_renderPass.getRenderPass();
                    framebufferCreateInfo.attachmentCount = 2;
                    framebufferCreateInfo.pAttachments = attachments;
                    framebufferCreateInfo.width = m_swapChain.getExtent().width;
                    framebufferCreateInfo.height = m_swapChain.getExtent().height;
                    framebufferCreateInfo.layers = 1;

                    m_framebuffers.resize(m_swapChain.getImagesViews().size());

                    for (size_t i = 0; i < m_framebuffers.size(); i++) {
                        attachments[0] = m_swapChain.getImagesViews()[i];
                        VK_CHECK_RESULT(vkCreateFramebuffer(
                                    m_device.getLogicalDevice(), 
                                    &framebufferCreateInfo, 
                                    nullptr, 
                                    &m_framebuffers[i]));
                    }
                }
            }

            virtual void createCommandPool() {}

            virtual void createTexture() {}

            virtual void createColorResources() {
                m_colorImageResource = VulkanImageResource(
                        m_device, 
                        m_swapChain, 
                        m_swapChain.getImageFormat(),  
                        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT 
                        | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
                        VK_IMAGE_ASPECT_COLOR_BIT);
                m_colorImageResource.create(m_device.getMsaaSamples());
            }

            virtual void createDepthResources() {
                m_depthImageResource = VulkanImageResource(
                        m_device, 
                        m_swapChain,
                        m_device.findDepthFormat(), 
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                        VK_IMAGE_ASPECT_DEPTH_BIT);
                m_depthImageResource.create(m_device.getMsaaSamples());
            }

            virtual void createVertexBuffer() {}

            virtual void createIndexBuffer() {}

            virtual void createCoordinateSystemUniformBuffers() {}
            virtual void createCommandBuffers() {}

            virtual void createSyncObjects() {
                m_syncObjects = VulkanSyncObjects(m_device, m_swapChain, 
                        MAX_FRAMES_IN_FLIGHT);
            }

            virtual void createDescriptorSetLayout() {}
            virtual void createDescriptorPool() {}
            virtual void createDescriptorSets() {}
            virtual void loadModel() {}

            virtual void OnUpdateUI(UI *ui) {}
    };

}

#define VULKAN_EXAMPLE_MAIN()					                    \
VulkanLearning::VulkanExample *vulkanExample;						\
int main(const int argc, const char *argv[])		                \
{\
    try \
    {\
        vulkanExample = new VulkanLearning::VulkanExample();        \
        vulkanExample->run();                                       \
    }\
    catch (const std::exception& e) \
    {\
        std::cerr << e.what() << std::endl;                         \
    }\
    return 0;                                                       \
} \

