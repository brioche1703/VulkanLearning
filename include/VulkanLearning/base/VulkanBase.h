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

#include "Vertex.hpp"
#include "camera/Camera.hpp"
#include "misc/FpsCounter.hpp"
#include "misc/VulkanDebug.hpp"
#include "VulkanDevice.hpp"
#include "VulkanInstance.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanSurface.hpp"
#include "VulkanRenderPass.hpp"
#include "misc/Inputs.hpp"
#include "window/Window.hpp"
#include "VulkanDescriptorSetLayout.hpp"
#include "VulkanShaderModule.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanDescriptorPool.hpp"
#include "VulkanDescriptorSets.hpp"
#include "VulkanSyncObjects.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanTexture.hpp"
#include "VulkanGraphicsPipeline.hpp"
#include "VulkanImageResource.hpp"
#include "VulkanTexture.hpp"
#include "misc/model/ModelObj.hpp"

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
        "VK_LAYER_MESA_overlay",
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    class VulkanBase {
        public:

            VulkanBase() {}
            virtual ~VulkanBase() {}

            void run() {
                initWindow();
                initCore();
                initVulkan();
                mainLoop();
                cleanup();
            }

            static std::vector<const char*> args;

        protected:
            Window m_window;
            Camera m_camera;
            FpsCounter m_fpsCounter;
            Inputs m_input;

            VulkanInstance *m_instance;
            VulkanDebug* m_debug = new VulkanDebug();

            VulkanDevice m_device;
            VulkanSurface m_surface;

            VulkanSwapChain m_swapChain;

            VulkanRenderPass m_renderPass;

            VulkanDescriptorSetLayout m_descriptorSetLayout;
            VulkanDescriptorPool m_descriptorPool;

            VulkanDescriptorSets m_descriptorSets;

            VulkanGraphicsPipeline m_graphicsPipeline;

            ModelObj m_model;

            VulkanBuffer m_vertexBuffer;
            VulkanBuffer m_indexBuffer;

            std::vector<VulkanBuffer> m_coordinateSystemUniformBuffers;

            std::vector<VulkanCommandBuffer> m_commandBuffers;

            VulkanSyncObjects m_syncObjects;

            size_t currentFrame = 0;
            bool framebufferResized = false;

            VulkanTexture* m_texture;
            VulkanTexture2D* m_texture2d;

            VulkanImageResource* m_colorImageResource;
            VulkanImageResource* m_depthImageResource;

            virtual void initWindow() {}

            virtual void initCore() {}

            virtual void initVulkan() {}

            virtual void mainLoop() {}

            virtual void drawFrame() {}

            virtual void cleanup() {}

            virtual void createSurface() {}

            virtual void recreateSwapChain() {}

            virtual void cleanupSwapChain() {}

            virtual void  createInstance() {}

            virtual void  createDebug() {}

            virtual void  createDevice() {}

            virtual void  createSwapChain() {}

            virtual void createRenderPass() {}

            virtual void createGraphicsPipeline() {}

            virtual void createFramebuffers() {}

            virtual void createCommandPool() {}

            virtual void createTexture() {}

            virtual void createColorResources() {}

            virtual void createDepthResources() {}

            virtual void createVertexBuffer() {}

            virtual void createIndexBuffer() {}

            virtual void createCoordinateSystemUniformBuffers() {}
            virtual void createCommandBuffers() {}

            virtual void createSyncObjects() {}

            virtual void createDescriptorSetLayout() {}
            virtual void createDescriptorPool() {}
            virtual void createDescriptorSets() {}
            virtual void updateCamera(uint32_t currentImage) {}
            virtual void loadModel() {}
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

