#include "../../../include/VulkanLearning/base/VulkanBase.h"

#include <cstring>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <vulkan/vulkan_core.h>
#include <random>

float rnd() {
    return ((float) rand() / (RAND_MAX));
}

namespace VulkanLearning {

    const std::string MODEL_PATH = "./src/models/sphere.obj";


    class VulkanExample : public VulkanBase {
        private:

        public:
            VulkanExample() {
            }
            ~VulkanExample() {}

            void run() {
                VulkanBase::run();
            }

            struct  SpherePushConstantData {
                glm::vec4 color;
                glm::vec4 position;
            };

            std::array<SpherePushConstantData, 16> m_spheres;

        private:

            void initWindow() override {
                m_window = new Window("Simple Triangle", WIDTH, HEIGHT);
                m_window->init();
            }

            void initCore() override {
                m_camera = new Camera(glm::vec3(0.0f, 0.0f, 20.0f));
                m_fpsCounter = new FpsCounter();
                m_input = new Inputs(m_window->getWindow(), m_camera, m_fpsCounter);

                glfwSetKeyCallback(m_window->getWindow() , m_input->keyboard_callback);
                glfwSetScrollCallback(m_window->getWindow() , m_input->scroll_callback);
                glfwSetCursorPosCallback(m_window->getWindow() , m_input->mouse_callback);

                glfwSetInputMode(m_window->getWindow() , GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            void initVulkan() override {
                createInstance();
                createDebug();
                createSurface();
                createDevice();
                createSwapChain();
                createRenderPass();

                createDescriptorSetLayout();

                createGraphicsPipeline();

                createCommandPool();
                createDepthResources();
                createFramebuffers();

                loadModel();

                createVertexBuffer();
                createIndexBuffer();

                createCoordinateSystemUniformBuffers();

                createDescriptorPool();
                createDescriptorSets();

                setupSpheres();
                
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
                    vkWaitForFences(m_device->getLogicalDevice(), 1, 
                            &m_syncObjects->getImagesInFlight()[imageIndex], VK_TRUE, UINT64_MAX);
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

            void cleanup() override {
                cleanupSwapChain();

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
                createDepthResources();

                createFramebuffers();

                createCoordinateSystemUniformBuffers();
                createDescriptorPool();
                createDescriptorSets();
                createCommandBuffers();
            }

            void cleanupSwapChain() override {
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

            void  createInstance() override {
                m_instance = new VulkanInstance("Push Constants",
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
                        enableValidationLayers, validationLayers, 1);
            }

            void  createSwapChain() override {
                m_swapChain = new VulkanSwapChain(m_window, m_device, m_surface);
            }

            void createRenderPass() override {
                VkAttachmentDescription colorAttachment{};
                colorAttachment.format = m_swapChain->getImageFormat();
                colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                VkAttachmentDescription depthAttachment{};
                depthAttachment.format = m_device->findDepthFormat();
                depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentReference colorAttachmentRef{};
                colorAttachmentRef.attachment = 1;
                colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentReference depthAttachmentRef{};
                depthAttachmentRef.attachment = 0;
                depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments = &colorAttachmentRef;
                subpass.pDepthStencilAttachment = &depthAttachmentRef;

                const std::vector<VkAttachmentDescription> attachments = 
                    { depthAttachment, colorAttachment };

                m_renderPass = new VulkanRenderPass(m_swapChain, m_device);
                m_renderPass->create(attachments, subpass);
            }

            void createGraphicsPipeline() override {
                m_graphicsPipeline = new VulkanGraphicsPipeline(m_device,
                        m_swapChain, m_renderPass, m_descriptorSetLayout);

                VulkanShaderModule vertShaderModule = 
                    VulkanShaderModule("src/shaders/pushConstantsVert.spv", m_device);
                VulkanShaderModule fragShaderModule = 
                    VulkanShaderModule("src/shaders/pushConstantsFrag.spv", m_device);

                VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
                vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

                auto bindingDescription = VertexTextured::getBindingDescription();
                auto attributeDescriptions = VertexTextured::getAttributeDescriptions();

                vertexInputInfo.vertexBindingDescriptionCount = 1;
                vertexInputInfo.vertexAttributeDescriptionCount = 
                    static_cast<uint32_t>(attributeDescriptions.size());
                vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
                vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

                VkPipelineDepthStencilStateCreateInfo depthStencil{};
                depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthStencil.depthTestEnable = VK_TRUE;
                depthStencil.depthWriteEnable = VK_TRUE;
                depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
                depthStencil.depthBoundsTestEnable = VK_FALSE;
                depthStencil.stencilTestEnable = VK_FALSE;

                VkPushConstantRange pushConstantRange{};
                pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                pushConstantRange.offset = 0;
                pushConstantRange.size = sizeof(SpherePushConstantData);

                VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
                pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutInfo.setLayoutCount = 1;
                pipelineLayoutInfo.pSetLayouts = 
                    m_descriptorSetLayout->getDescriptorSetLayoutPointer();
                pipelineLayoutInfo.pushConstantRangeCount = 1;
                pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

                m_graphicsPipeline->create(
                        vertShaderModule, fragShaderModule, 
                        vertexInputInfo, pipelineLayoutInfo, &depthStencil);
            }

            void createFramebuffers() override {
                const std::vector<VkImageView> attachments {
                    m_depthImageResource->getImageView()
                };

                m_swapChain->createFramebuffers(m_renderPass->getRenderPass(),
                    attachments);
            }

            void createCommandPool() override {
                m_commandPool = new VulkanCommandPool(m_device);
            }

            void createDepthResources() override {
                m_depthImageResource = new VulkanImageResource(m_device, 
                        m_swapChain, m_commandPool, 
                        m_device->findDepthFormat(), 
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                        VK_IMAGE_ASPECT_DEPTH_BIT);
                m_depthImageResource->create();
            }

            void createVertexBuffer() override {
                m_vertexBuffer = new VulkanBuffer(m_device, m_commandPool);
                m_vertexBuffer->createWithStagingBuffer(m_model->getVerticies(),
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            }

            void createIndexBuffer() override {
                m_indexBuffer = new VulkanBuffer(m_device, m_commandPool);
                m_indexBuffer->createWithStagingBuffer(m_model->getIndicies(),
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            }

            void createCoordinateSystemUniformBuffers() override {
                VkDeviceSize bufferSize = sizeof(CoordinatesSystemUniformBufferObject);

                m_coordinateSystemUniformBuffers.resize(m_swapChain->getImages().size());

                for (size_t i = 0; i < m_swapChain->getImages().size(); i++) {
                    m_coordinateSystemUniformBuffers[i] = new VulkanBuffer(m_device, m_commandPool);
                    m_coordinateSystemUniformBuffers[i]->createBuffer(
                            bufferSize, 
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                            *m_coordinateSystemUniformBuffers[i]->getBufferPointer(), 
                            *m_coordinateSystemUniformBuffers[i]->getBufferMemoryPointer()
                            );
                }
            }

            void createCommandBuffers() override {
                m_commandBuffers = new VulkanCommandBuffers(m_device, 
                        m_swapChain, m_commandPool, m_renderPass, m_vertexBuffer,
                        m_indexBuffer, static_cast<uint32_t>(m_model->getIndicies().size()),
                        m_graphicsPipeline->getGraphicsPipeline(),
                        m_graphicsPipeline->getPipelineLayout(), 
                        m_descriptorSets);

                m_commandBuffers->create();

                std::array<VkClearValue, 2> clearValues{};
                clearValues[1].color = {0.0f, 0.0f, 0.0f, 1.0f};
                clearValues[0].depthStencil = {1.0f, 0};

                for (size_t i = 0; i < m_commandBuffers->getCommandBuffers().size(); i++) {
                    VkCommandBufferBeginInfo beginInfo{};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = 0;
                    beginInfo.pInheritanceInfo = nullptr;

                    if (vkBeginCommandBuffer(m_commandBuffers->getCommandBuffers()[i], &beginInfo) != VK_SUCCESS) {
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

                    vkCmdBeginRenderPass(m_commandBuffers->getCommandBuffers()[i], 
                            &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdBindPipeline(m_commandBuffers->getCommandBuffers()[i], 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->getGraphicsPipeline());

                    VkBuffer vertexBuffers[] = { m_vertexBuffer->getBuffer() };
                    VkDeviceSize offsets[] = {0};
                    vkCmdBindVertexBuffers(m_commandBuffers->getCommandBuffers()[i], 
                            0, 1, vertexBuffers, offsets);
                    vkCmdBindIndexBuffer(m_commandBuffers->getCommandBuffers()[i], 
                            m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

                    vkCmdBindDescriptorSets(m_commandBuffers->getCommandBuffers()[i], 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_graphicsPipeline->getPipelineLayout(), 0, 1, 
                            &m_descriptorSets->getDescriptorSets()[i], 
                            0, nullptr);

                    uint32_t sphereCount = static_cast<uint32_t>(m_spheres.size());
                    for (uint32_t j = 0; j < sphereCount; j++) {
                        vkCmdPushConstants(m_commandBuffers->getCommandBuffers()[i], 
                                m_graphicsPipeline->getPipelineLayout(), 
                                VK_SHADER_STAGE_VERTEX_BIT, 0, 
                                sizeof(SpherePushConstantData), 
                                &m_spheres[j]);

                    vkCmdDrawIndexed(m_commandBuffers->getCommandBuffers()[i], 
                            static_cast<uint32_t>(m_model->getIndicies().size())
                            , 1, 0, 0, 0);
                    }

                    vkCmdEndRenderPass(m_commandBuffers->getCommandBuffers()[i]);

                    if (vkEndCommandBuffer(m_commandBuffers->getCommandBuffers()[i]) != VK_SUCCESS) {
                        throw std::runtime_error("Recording of a command buffer failed!");
                    }
                }
            }

            void createSyncObjects() override {
                m_syncObjects = new VulkanSyncObjects(m_device, m_swapChain, 
                        MAX_FRAMES_IN_FLIGHT);
            }

            void createDescriptorSetLayout() override {
                m_descriptorSetLayout = new VulkanDescriptorSetLayout(m_device);

                VkDescriptorSetLayoutBinding uboLayoutBinding{};
                uboLayoutBinding.binding = 0;
                uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uboLayoutBinding.descriptorCount = 1;
                uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                uboLayoutBinding.pImmutableSamplers = nullptr;

                std::vector<VkDescriptorSetLayoutBinding> bindings = 
                {uboLayoutBinding};

                m_descriptorSetLayout->create(bindings);
            }

            void createDescriptorPool() override {
                m_descriptorPool = new VulkanDescriptorPool(m_device, m_swapChain);

                std::vector<VkDescriptorPoolSize> poolSizes = std::vector<VkDescriptorPoolSize>(1);
                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = static_cast<uint32_t>(
                        m_swapChain->getImages().size());

                m_descriptorPool->create(poolSizes);
            }

            void createDescriptorSets() override {
                std::vector<std::vector<VulkanBuffer*>> ubos{
                    m_coordinateSystemUniformBuffers,
                };

                std::vector<VkDeviceSize> ubosSizes{
                    sizeof(CoordinatesSystemUniformBufferObject),
                };

                m_descriptorSets = new VulkanDescriptorSets(
                        m_device, m_swapChain,
                        m_descriptorSetLayout, m_descriptorPool,
                        ubos, ubosSizes);

                std::vector<VkWriteDescriptorSet> descriptorWrites = 
                    std::vector<VkWriteDescriptorSet>(1);

                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[0].descriptorCount = 1;

                VkDescriptorBufferInfo bufferInfoAux{};
                bufferInfoAux.offset = 0;
                
                m_descriptorSets->create(descriptorWrites);
            }

            void updateCamera(uint32_t currentImage) override {
                CoordinatesSystemUniformBufferObject ubo{};

                ubo.model = glm::mat4(1.0f);
                ubo.model = glm::scale(ubo.model, glm::vec3(0.5f, 0.5f, 0.5f));

                ubo.view = m_camera->getViewMatrix();

                ubo.proj = glm::perspective(glm::radians(m_camera->getZoom()), 
                        m_swapChain->getExtent().width / (float) m_swapChain->getExtent().height, 
                        0.1f,  100.0f);

                ubo.proj[1][1] *= -1;

                m_coordinateSystemUniformBuffers[currentImage]->map();
                memcpy(m_coordinateSystemUniformBuffers[currentImage]->getMappedMemory(), 
                        &ubo, sizeof(ubo));
                m_coordinateSystemUniformBuffers[currentImage]->unmap();
            }

            void loadModel() override {
                m_model = new ModelObj(MODEL_PATH);
            }

            void setupSpheres() {
                for (uint32_t i = 0; i < m_spheres.size(); i++) {
                    m_spheres[i].color = glm::vec4(rnd(), rnd(), rnd(), 1.0f);

                    const float rad = glm::radians(i * 360.0f 
                            / static_cast<uint32_t>(m_spheres.size()));

                    m_spheres[i].position = glm::vec4(glm::vec3(sin(rad), cos(rad),
                                0.0f) * 3.5f, 1.0f);
                }
            }

    };

}

VULKAN_EXAMPLE_MAIN()