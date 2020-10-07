#include "../../../include/VulkanLearning/base/VulkanBase.h"

namespace VulkanLearning {


    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    const std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0
    };

    class VulkanExample : public VulkanBase {
        public:
            VulkanExample() {}
            ~VulkanExample() {}

            void run() {
                VulkanBase::run();
            }
        private:

            void initWindow() override {
                m_window = new Window("Simple Triangle", WIDTH, HEIGHT);
                m_window->init();
            }

            void initCore() override {
                m_camera = new Camera(glm::vec3(0.0f, 2.0f, 5.0f));
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

                createDescriptorSetLayout();

                createGraphicsPipeline();

                createCommandPool();
                createFramebuffers();

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

                createFramebuffers();

                createCoordinateSystemUniformBuffers();
                createDescriptorPool();
                createDescriptorSets();
                createCommandBuffers();
            }

            void cleanupSwapChain() override {
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
                m_instance = new VulkanInstance("Simple Triangle", enableValidationLayers, 
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

                VkAttachmentReference colorAttachmentRef{};
                colorAttachmentRef.attachment = 0;
                colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments = &colorAttachmentRef;

                const std::vector<VkAttachmentDescription> attachments = 
                    { colorAttachment };

                m_renderPass = new VulkanRenderPass(m_swapChain, m_device);
                m_renderPass->create(attachments, subpass);
            }

            void createGraphicsPipeline() override {
                m_graphicsPipeline = new VulkanGraphicsPipeline(m_device,
                        m_swapChain, m_renderPass, m_descriptorSetLayout);

                VulkanShaderModule vertShaderModule = 
                    VulkanShaderModule("src/shaders/simpleTriangleShaderVert.spv", m_device);
                VulkanShaderModule fragShaderModule = 
                    VulkanShaderModule("src/shaders/simpleTriangleShaderFrag.spv", m_device);

                VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
                vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

                auto bindingDescription = Vertex::getBindingDescription();
                auto attributeDescriptions = Vertex::getAttributeDescriptions();

                vertexInputInfo.vertexBindingDescriptionCount = 1;
                vertexInputInfo.vertexAttributeDescriptionCount = 
                    static_cast<uint32_t>(attributeDescriptions.size());
                vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
                vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

                VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
                pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutInfo.setLayoutCount = 1;
                pipelineLayoutInfo.pSetLayouts = 
                    m_descriptorSetLayout->getDescriptorSetLayoutPointer();

                m_graphicsPipeline->create(vertShaderModule, fragShaderModule, 
                        vertexInputInfo, pipelineLayoutInfo);
            }

            void createFramebuffers() override {
                const std::vector<VkImageView> attachments {};

                m_swapChain->createFramebuffers(m_renderPass->getRenderPass(),
                    attachments);
            }

            void createCommandPool() override {
                m_commandPool = new VulkanCommandPool(m_device);
            }

            void createVertexBuffer() override {
                m_vertexBuffer = new VulkanBuffer(m_device, m_commandPool);
                m_vertexBuffer->createWithStagingBuffer(vertices, 
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            }

            void createIndexBuffer() override {
                m_indexBuffer = new VulkanBuffer(m_device, m_commandPool);
                m_indexBuffer->createWithStagingBuffer(indices,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            }

            void createCoordinateSystemUniformBuffers() override {
                VkDeviceSize bufferSize = sizeof(CoordinatesSystemUniformBufferObject);

                m_coordinateSystemUniformBuffers.resize(m_swapChain->getImages().size());

                for (size_t i = 0; i < m_swapChain->getImages().size(); i++) {
                    m_coordinateSystemUniformBuffers[i] = new VulkanBuffer(m_device, m_commandPool);
                    m_coordinateSystemUniformBuffers[i]->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, *m_coordinateSystemUniformBuffers[i]->getBufferPointer(), *m_coordinateSystemUniformBuffers[i]->getBufferMemoryPointer());
                }
            }

            void createCommandBuffers() override {
                m_commandBuffers = new VulkanCommandBuffers(m_device, 
                        m_swapChain, m_commandPool, m_renderPass, m_vertexBuffer,
                        m_indexBuffer, static_cast<uint32_t>(indices.size()),
                        m_graphicsPipeline->getGraphicsPipeline(),
                        m_graphicsPipeline->getPipelineLayout(), 
                        m_descriptorSets);

                std::array<VkClearValue, 1> clearValues{};
                clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
                m_commandBuffers->create(clearValues);
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
                m_descriptorSets = new VulkanDescriptorSets(m_device, m_swapChain,
                        m_descriptorSetLayout, m_descriptorPool,
                        m_coordinateSystemUniformBuffers, sizeof(CoordinatesSystemUniformBufferObject));

                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.offset = 0;

                std::vector<VkWriteDescriptorSet> descriptorWrites = 
                    std::vector<VkWriteDescriptorSet>(1);

                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[0].descriptorCount = 1;
                
                m_descriptorSets->create(bufferInfo, descriptorWrites);
            }

            void updateCamera(uint32_t currentImage) override {
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

    };

}

VULKAN_EXAMPLE_MAIN()
