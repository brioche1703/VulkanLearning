#include "VulkanBase.hpp"
#include "VulkanglTFModel.hpp"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

namespace VulkanLearning {

    class VulkanExample : public VulkanBase {

        private:
            VulkanglTFModel glTFModel;

            uint32_t m_msaaSamples = 64;

            struct ubo {
                VulkanBuffer buffer;
                struct Values {
                    glm::mat4 projection;
                    glm::mat4 model;
                    glm::vec4 lightPos = glm::vec4(3.0f, 3.0f, -3.0f, 1.0f);
                } values;
            } ubo;

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
            void initWindow() override {
                m_window = Window("Vulkan", WIDTH, HEIGHT);
                m_window.init();
            }

            void initCore() override {
                m_camera = Camera(glm::vec3(0.0f, 0.0f, 5.0f));
                m_fpsCounter = FpsCounter();
                m_input = Inputs(m_window.getWindow(), &m_camera, &m_fpsCounter, &m_ui);

                glfwSetKeyCallback(m_window.getWindow() , m_input.keyboard_callback);
                glfwSetScrollCallback(m_window.getWindow() , m_input.scroll_callback);
                glfwSetMouseButtonCallback(m_window.getWindow() , m_input.mouse_button_callback);

                glfwSetInputMode(m_window.getWindow() , GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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

                createUniformBuffers();
                createDescriptorPool();
                createDescriptorSets();
                createCommandBuffers();
                createSyncObjects();
            }

            void mainLoop() override {
                while (!glfwWindowShouldClose(m_window.getWindow() )) {
                    glfwPollEvents();
                    m_input.processKeyboardInput();
                    m_fpsCounter.update();
                    updateUI();
                    drawFrame();
                }

                vkDeviceWaitIdle(m_device.getLogicalDevice());
            }

            void drawFrame() override {
                vkWaitForFences(m_device.getLogicalDevice(), 1, 
                        &m_syncObjects.getInFlightFences()[currentFrame], VK_TRUE, UINT64_MAX);

                uint32_t imageIndex;

                VkResult result = vkAcquireNextImageKHR(m_device.getLogicalDevice(), 
                        m_swapChain.getSwapChain(), UINT64_MAX, 
                        m_syncObjects.getImageAvailableSemaphores()[currentFrame], 
                        VK_NULL_HANDLE, &imageIndex);

                if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                    recreateSwapChain();
                    return;
                } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                    throw std::runtime_error("Presentation of one image of the swap chain failed!");
                }

                if (m_syncObjects.getImagesInFlight()[imageIndex] != VK_NULL_HANDLE) {
                    vkWaitForFences(m_device.getLogicalDevice(), 1, &m_syncObjects.getImagesInFlight()[imageIndex], VK_TRUE, UINT64_MAX);
                }

                m_syncObjects.getImagesInFlight()[imageIndex] = m_syncObjects.getInFlightFences()[currentFrame];

                updateUniformBuffers();

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                VkSemaphore waitSemaphore[] = {m_syncObjects.getImageAvailableSemaphores()[currentFrame]};
                VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphore;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = m_commandBuffers[imageIndex].getCommandBufferPointer();

                VkSemaphore signalSemaphores[] = {m_syncObjects.getRenderFinishedSemaphores()[currentFrame]};
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                vkResetFences(m_device.getLogicalDevice(), 1, &m_syncObjects.getInFlightFences()[currentFrame]);

                if (vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, m_syncObjects.getInFlightFences()[currentFrame]) != VK_SUCCESS) {
                    throw std::runtime_error("Command buffer sending failed!");
                }

                VkPresentInfoKHR presentInfo{};
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = signalSemaphores;

                VkSwapchainKHR swapChains[] = {m_swapChain.getSwapChain()};
                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = swapChains;
                presentInfo.pImageIndices = &imageIndex;
                presentInfo.pResults = nullptr;

                result = vkQueuePresentKHR(m_device.getPresentQueue(), &presentInfo);

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


                m_descriptorSetLayouts.matrices->cleanup();
                m_descriptorSetLayouts.textures->cleanup();

                ubo.buffer.cleanup();
                glTFModel.~VulkanglTFModel();

                m_syncObjects.cleanup();

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

            void createSurface() override {
                m_surface = VulkanSurface();
                m_surface.create(m_window, *m_instance);
            }

            void recreateSwapChain() override {
                int width = 0, height = 0;
                while (width == 0 || height == 0) {
                    glfwGetFramebufferSize(m_window.getWindow() , &width, &height);
                    glfwWaitEvents();
                }

                vkDeviceWaitIdle(m_device.getLogicalDevice());

                cleanupSwapChain();

                m_swapChain.create();

                createRenderPass();
                createGraphicsPipeline();
                createColorResources();
                createDepthResources();

                createFramebuffers();

                createUniformBuffers();
                createDescriptorPool();
                createDescriptorSets();
                createCommandBuffers();
                m_ui.resize(m_swapChain.getExtent().width, m_swapChain.getExtent().height);
            }

            void cleanupSwapChain() override {
                m_colorImageResource.cleanup();
                m_depthImageResource.cleanup();

                m_swapChain.cleanFramebuffers();

                vkFreeCommandBuffers(m_device.getLogicalDevice(), 
                        m_device.getCommandPool(), 
                        static_cast<uint32_t>(
                            m_commandBuffers.size()), 
                        m_commandBuffers.data()->getCommandBufferPointer());

                vkDestroyPipeline(m_device.getLogicalDevice(), 
                        m_graphicsPipeline.getGraphicsPipeline(), nullptr);
                vkDestroyPipelineLayout(m_device.getLogicalDevice(), 
                        m_graphicsPipeline.getPipelineLayout(), nullptr);
                vkDestroyRenderPass(m_device.getLogicalDevice(), 
                        m_renderPass.getRenderPass(), nullptr);

                m_swapChain.destroyImageViews();
                vkDestroySwapchainKHR(m_device.getLogicalDevice(), m_swapChain.getSwapChain(), nullptr);

                vkDestroyDescriptorPool(m_device.getLogicalDevice(), 
                        m_descriptorPool.getDescriptorPool(), nullptr);
            }

            void  createInstance() override {
                m_instance = new VulkanInstance(
                        "glTF Loading",
                        enableValidationLayers, 
                        validationLayers, m_debug);
            }

            void  createDebug() override {
                m_debug = new VulkanDebug(m_instance->getInstance(), 
                        enableValidationLayers);
            }

            void  createDevice() override {
                m_device = VulkanDevice(m_instance->getInstance(), m_surface.getSurface(), 
                        deviceExtensions,
                        enableValidationLayers, validationLayers, m_msaaSamples);
            }

            void  createSwapChain() override {
                m_swapChain = VulkanSwapChain(m_window, m_device, m_surface);
            }

            void createRenderPass() override {
                VkAttachmentDescription colorAttachment{};
                colorAttachment.format = m_swapChain.getImageFormat();
                colorAttachment.samples = m_device.getMsaaSamples();
                colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentDescription depthAttachment{};
                depthAttachment.format = m_device.findDepthFormat();
                depthAttachment.samples = m_device.getMsaaSamples();
                depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentDescription colorAttachmentResolve{};
                colorAttachmentResolve.format = m_swapChain.getImageFormat();
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

                m_renderPass = VulkanRenderPass(m_swapChain, m_device);
                m_renderPass.create(attachments, subpass);
            }

            void createGraphicsPipeline() override {
                m_graphicsPipeline = VulkanGraphicsPipeline(m_device,
                        m_swapChain, m_renderPass);

                VulkanShaderModule vertShaderModule = 
                    VulkanShaderModule("src/shaders/glTFLoadingVert.spv", &m_device, VK_SHADER_STAGE_VERTEX_BIT);
                VulkanShaderModule fragShaderModule = 
                    VulkanShaderModule("src/shaders/glTFLoadingFrag.spv", &m_device, VK_SHADER_STAGE_FRAGMENT_BIT);

                VkVertexInputBindingDescription vertexInputBindingDescription = {};
                vertexInputBindingDescription.binding = 0;
                vertexInputBindingDescription.stride = sizeof(VulkanglTFModel::Vertex);
                vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                std::vector<VkVertexInputBindingDescription> vertexInputBindingsDescription = {
                    vertexInputBindingDescription
                };
                
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
                vertexAttributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
                vertexAttributeDescription[2].location = 2;
                vertexAttributeDescription[2].offset = offsetof(VulkanglTFModel::Vertex, uv);

                vertexAttributeDescription[3].binding = 0;
                vertexAttributeDescription[3].format = VK_FORMAT_R32G32B32_SFLOAT;
                vertexAttributeDescription[3].location = 3;
                vertexAttributeDescription[3].offset = offsetof(VulkanglTFModel::Vertex, color);

                VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
                vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindingsDescription.size());
                vertexInputInfo.pVertexBindingDescriptions = vertexInputBindingsDescription.data();
                vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescription.size());
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

                m_graphicsPipeline.create(
                        vertShaderModule, 
                        fragShaderModule,
                        vertexInputInfo, 
                        pipelineLayoutInfo, 
                        &depthStencil);
            }

            void createFramebuffers() override {
                const std::vector<VkImageView> attachments {
                    m_colorImageResource.getImageView(),
                        m_depthImageResource.getImageView()
                };

                m_swapChain.createFramebuffers(m_renderPass.getRenderPass(), 
                        attachments);
            }

            void createColorResources() override {
                m_colorImageResource = VulkanImageResource(m_device, 
                        m_swapChain, 
                        m_swapChain.getImageFormat(),  
                        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
                        | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
                        VK_IMAGE_ASPECT_COLOR_BIT);
                m_colorImageResource.create();
            }

            void createDepthResources() override {
                m_depthImageResource = VulkanImageResource(m_device, 
                        m_swapChain,
                        m_device.findDepthFormat(), 
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                        VK_IMAGE_ASPECT_DEPTH_BIT);
                m_depthImageResource.create();
            }

            void createUniformBuffers() {
                ubo.buffer = VulkanBuffer(m_device);
                ubo.buffer.createBuffer(sizeof(ubo.values), 
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                ubo.buffer.map();

                updateUniformBuffers();
            }

            void createCommandBuffers() override {
                m_commandBuffers.resize(m_swapChain.getImages().size());

                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.commandPool = m_device.getCommandPool();
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocInfo.commandBufferCount = (uint32_t) m_commandBuffers.size();

                if (vkAllocateCommandBuffers(m_device.getLogicalDevice(), &allocInfo, m_commandBuffers.data()->getCommandBufferPointer()) != VK_SUCCESS) {
                    throw std::runtime_error("Command buffers allocation failed!");
                }

                VkCommandBufferBeginInfo cmdBufInfo = {};
                cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

                VkClearValue clearValues[2];
                clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
                clearValues[1].depthStencil = { 1.0f, 0 };

                VkViewport viewport = {};
                viewport.width = m_swapChain.getExtent().width;
                viewport.height = m_swapChain.getExtent().height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                VkRect2D scissor = {};
                scissor.extent.width = m_swapChain.getExtent().width;
                scissor.extent.height = m_swapChain.getExtent().height;
                scissor.offset.x = 0;
                scissor.offset.y = 0;

                for (int32_t i = 0; i < m_commandBuffers.size(); ++i)
                {
                    VkCommandBufferBeginInfo beginInfo{};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = 0;
                    beginInfo.pInheritanceInfo = nullptr;

                    if (vkBeginCommandBuffer(m_commandBuffers[i].getCommandBuffer(), &beginInfo) != VK_SUCCESS) {
                        throw std::runtime_error("Begin recording of a command buffer failed!");
                    }
                    VkRenderPassBeginInfo renderPassBeginInfo = {};
                    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassBeginInfo.renderPass = m_renderPass.getRenderPass();
                    renderPassBeginInfo.renderArea.offset.x = 0;
                    renderPassBeginInfo.renderArea.offset.y = 0;
                    renderPassBeginInfo.renderArea.extent.width = m_swapChain.getExtent().width;
                    renderPassBeginInfo.renderArea.extent.height = m_swapChain.getExtent().height;
                    renderPassBeginInfo.clearValueCount = 2;
                    renderPassBeginInfo.pClearValues = clearValues;
                    renderPassBeginInfo.framebuffer = m_swapChain.getFramebuffers()[i];

                    vkCmdBeginRenderPass(m_commandBuffers[i].getCommandBuffer(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdSetViewport(m_commandBuffers[i].getCommandBuffer(), 0, 1, &viewport);
                    vkCmdSetScissor(m_commandBuffers[i].getCommandBuffer(), 0, 1, &scissor);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_graphicsPipeline.getGraphicsPipeline());

                    vkCmdBindDescriptorSets(m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_graphicsPipeline.getPipelineLayout(), 
                            0, 
                            1, 
                            &m_descriptorSets.getDescriptorSets()[i], 
                            0, 
                            nullptr);


                    glTFModel.draw(m_commandBuffers[i].getCommandBuffer(), m_graphicsPipeline.getPipelineLayout());

                    drawUI(m_commandBuffers[i].getCommandBuffer());

                    vkCmdEndRenderPass(m_commandBuffers[i].getCommandBuffer());
                    if (vkEndCommandBuffer(m_commandBuffers[i].getCommandBuffer()) != VK_SUCCESS) {
                        throw std::runtime_error("Recording of a command buffer failed!");
                    }
                }
            }

            void createSyncObjects() override {
                m_syncObjects = VulkanSyncObjects(m_device, m_swapChain, 
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
                uboLayoutBinding.pImmutableSamplers = nullptr;

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
                m_descriptorPool = VulkanDescriptorPool(m_device, m_swapChain);

                std::vector<VkDescriptorPoolSize> poolSizes = std::vector<VkDescriptorPoolSize>(2);

                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = static_cast<uint32_t>(
                        m_swapChain.getImages().size());

                poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[1].descriptorCount = 
                    static_cast<uint32_t>(glTFModel.images.size());

                const uint32_t maxSetCount = 
                    static_cast<uint32_t>(m_swapChain.getImages().size())
                    * static_cast<uint32_t>(glTFModel.images.size()) + 1;

                m_descriptorPool.create(poolSizes, maxSetCount);
            }

            void createDescriptorSets() override {
                m_descriptorSets = VulkanDescriptorSets(
                        m_device, 
                        *m_descriptorSetLayouts.matrices, 
                        m_descriptorPool);

                m_descriptorSets.create(static_cast<uint32_t>(m_swapChain.getImages().size()));

                for (size_t i = 0; i < m_swapChain.getImages().size(); i++) {
                    std::vector<VkWriteDescriptorSet> descriptorWrites(1);

                    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrites[0].dstBinding = 0;
                    descriptorWrites[0].dstArrayElement = 0;
                    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    descriptorWrites[0].descriptorCount = 1;
                    descriptorWrites[0].pBufferInfo = ubo.buffer.getDescriptorPointer();

                    m_descriptorSets.update(descriptorWrites, i);
                }

                for (auto& image : glTFModel.images) {
                    VkDescriptorSetAllocateInfo allocInfo{};
                    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    allocInfo.descriptorPool = m_descriptorPool.getDescriptorPool();
                    allocInfo.pSetLayouts = m_descriptorSetLayouts.textures->getDescriptorSetLayoutPointer();
                    allocInfo.descriptorSetCount = 1;

                    if (vkAllocateDescriptorSets(m_device.getLogicalDevice(), &allocInfo, &image.descriptorSet) != VK_SUCCESS) {
                        throw std::runtime_error("Descriptor set allocation failed!");
                    }

                    VkWriteDescriptorSet descriptorWrite;
                    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrite.dstSet = image.descriptorSet;
                    descriptorWrite.dstBinding = 0;
                    descriptorWrite.dstArrayElement = 0;
                    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    descriptorWrite.descriptorCount = 1;
                    descriptorWrite.pImageInfo = image.texture.getDescriptorPointer();
                    descriptorWrite.pNext = VK_NULL_HANDLE;

                    vkUpdateDescriptorSets(
                            m_device.getLogicalDevice(), 
                            1, 
                            &descriptorWrite, 
                            0, 
                            nullptr);
                }
            }

            void updateUniformBuffers() {
                ubo.values.projection = glm::perspective(glm::radians(m_camera.getZoom()), 
                        m_swapChain.getExtent().width / (float) m_swapChain.getExtent().height, 
                        0.1f,  100.0f);
                ubo.values.projection[1][1] *= -1;
                ubo.values.model = m_camera.getViewMatrix();
                memcpy(ubo.buffer.getMappedMemory(), &ubo.values, sizeof(ubo.values));
            }

            void loadglTFFile(std::string filename) {
                tinygltf::Model glTFInput;
                tinygltf::TinyGLTF gltfContext;
                std::string error, warning;

                bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, filename);

                glTFModel.device = &m_device;
                glTFModel.copyQueue = m_device.getGraphicsQueue();

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
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        vertexBuffer.data());

                VulkanBuffer indexStagingBuffer(m_device);
                indexStagingBuffer.createBuffer(
                        indexBufferSize, 
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        indexBuffer.data());

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
                copyCmd.create(&m_device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

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

                copyCmd.flushCommandBuffer(&m_device, true);

                vertexStagingBuffer.cleanup();
                indexStagingBuffer.cleanup();
            }

            void loadAssets() {
                loadglTFFile("src/models/FlightHelmet/glTF/FlightHelmet.gltf");
            }

            void OnUpdateUI (UI *ui) override {
                if (ui->header("Settings")) {
                    if (ui->checkBox("Wireframe", &m_wireframe)) {
                        createCommandBuffers();
                    }
                }
            }

    };

}

VULKAN_EXAMPLE_MAIN()
