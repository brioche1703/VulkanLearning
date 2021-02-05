#include "VulkanBase.hpp"
#include <cstring>
#include <glm/ext/matrix_transform.hpp>
#include <vulkan/vulkan_core.h>
#include <random>

namespace VulkanLearning {

    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}},

        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    };

    const std::vector<uint32_t> indices = {
        0, 1, 2, 2, 1, 3,
        3, 1, 4, 4, 5, 3,
        3, 5, 6, 6, 2, 3,
        6, 7, 2, 2, 7, 0,
        0, 7, 4, 4, 1, 0,
        7, 6, 4, 4, 6, 5,
    };

    struct DynUbo {
        glm::mat4* model;
    } uboDataDynamic;

#define NUM_OBJ 64

    void* alignedAlloc(size_t size, size_t alignment) {
        void* data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
        data = _aligned_malloc(size, alignment);
#else
        int res = posix_memalign(&data, alignment, size);
        if (res != 0)
            data = nullptr;
#endif
        return data;
    }

    void alignedFree(void* data) {
#if defined(_MSC_VER) || defined(__MINGW32__)
        _aligned_free(data);
#else
        free(data);
#endif
    }

    class VulkanExample : public VulkanBase {
        private:


            std::vector<VulkanBuffer> m_dynUbos;
            size_t m_dynamicAlignment;
            glm::vec3 m_rotations[NUM_OBJ];
            glm::vec3 m_rotationsSpeeds[NUM_OBJ];

            VulkanDescriptorSets m_descriptorSets;

            VkPipeline m_pipeline;
            VkPipelineLayout m_pipelineLayout;

        public:
            VulkanExample() {
            }
            ~VulkanExample() {
                m_descriptorSetLayout.cleanup();

                m_vertexBuffer.cleanup();
                m_indexBuffer.cleanup();

                for (size_t i = 0; i < m_swapChain.getImages().size(); i++) {
                    vkDestroyBuffer(m_device.getLogicalDevice(), 
                            m_coordinateSystemUniformBuffers[i].getBuffer(), 
                            nullptr);
                    vkFreeMemory(m_device.getLogicalDevice(), 
                            m_coordinateSystemUniformBuffers[i].getBufferMemory(), 
                            nullptr);
                    vkDestroyBuffer(m_device.getLogicalDevice(), 
                            m_dynUbos[i].getBuffer(), 
                            nullptr);
                    vkFreeMemory(m_device.getLogicalDevice(), 
                            m_dynUbos[i].getBufferMemory(), 
                            nullptr);
                }

                vkDestroyDescriptorPool(m_device.getLogicalDevice(), 
                        m_descriptorPool.getDescriptorPool(), nullptr);
            }

            void run() {
                VulkanBase::run();
            }
        private:

            void initVulkan() override {
                VulkanBase::initVulkan();

                m_window.setTitle("Dynamic Uniform Buffers");
                m_camera.setPosition(glm::vec3(0.0f, 0.0f, 20.0f));

                createVertexBuffer();
                createIndexBuffer();

                createDynamicUniformBuffers();
                createDescriptorSetLayout();
                createGraphicsPipeline();

                createCoordinateSystemUniformBuffers();
                createDescriptorPool();
                createDescriptorSets();
                createCommandBuffers();
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
                        &m_syncObjects.getInFlightFences()[m_currentFrame], VK_TRUE, UINT64_MAX);

                uint32_t imageIndex;

                VkResult result = vkAcquireNextImageKHR(m_device.getLogicalDevice(), 
                        m_swapChain.getSwapChain(), UINT64_MAX, 
                        m_syncObjects.getImageAvailableSemaphores()[m_currentFrame], 
                        VK_NULL_HANDLE, &imageIndex);

                if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                    recreateSwapChain();
                    return;
                } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                    throw std::runtime_error("Presentation of one image of the swap chain failed!");
                }

                if (m_syncObjects.getImagesInFlight()[imageIndex] != VK_NULL_HANDLE) {
                    vkWaitForFences(m_device.getLogicalDevice(), 1, 
                            &m_syncObjects.getImagesInFlight()[imageIndex], VK_TRUE, UINT64_MAX);
                }

                m_syncObjects.getImagesInFlight()[imageIndex] = m_syncObjects.getInFlightFences()[m_currentFrame];

                updateUniformBuffers(imageIndex);
                updateDynUbos(imageIndex);

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                VkSemaphore waitSemaphore[] = {m_syncObjects.getImageAvailableSemaphores()[m_currentFrame]};
                VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphore;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = m_commandBuffers[imageIndex].getCommandBufferPointer();

                VkSemaphore signalSemaphores[] = {m_syncObjects.getRenderFinishedSemaphores()[m_currentFrame]};
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                vkResetFences(m_device.getLogicalDevice(), 1, &m_syncObjects.getInFlightFences()[m_currentFrame]);

                if (vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, m_syncObjects.getInFlightFences()[m_currentFrame]) != VK_SUCCESS) {
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

                m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            }

            void createRenderPass() override {
                VkAttachmentDescription colorAttachment{};
                colorAttachment.format = m_swapChain.getImageFormat();
                colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                VkAttachmentDescription depthAttachment{};
                depthAttachment.format = m_device.findDepthFormat();
                depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentReference colorAttachmentRef{};
                colorAttachmentRef.attachment = 0;
                colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentReference depthAttachmentRef{};
                depthAttachmentRef.attachment = 1;
                depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments = &colorAttachmentRef;
                subpass.pDepthStencilAttachment = &depthAttachmentRef;

                const std::vector<VkAttachmentDescription> attachments = 
                    { colorAttachment, depthAttachment };

                m_renderPass = VulkanRenderPass(m_swapChain, m_device);
                m_renderPass.create(attachments, subpass);
            }

            void createGraphicsPipeline() override {
                VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
                pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutInfo.setLayoutCount = 1;
                pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayout.getDescriptorSetLayoutPointer();
                vkCreatePipelineLayout(
                        m_device.getLogicalDevice(), 
                        &pipelineLayoutInfo, 
                        nullptr, 
                        &m_pipelineLayout);

                VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
                inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                inputAssembly.flags = 0;
                inputAssembly.primitiveRestartEnable = VK_FALSE;

                VkPipelineRasterizationStateCreateInfo rasterizer = {};
                rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
                rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
                rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                rasterizer.flags = 0;
                rasterizer.depthClampEnable = VK_FALSE;
                rasterizer.lineWidth = 1.0f;

                VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
                colorBlendAttachment.colorWriteMask = 0xf;
                colorBlendAttachment.blendEnable = VK_FALSE;

                VkPipelineColorBlendStateCreateInfo colorBlending{};
                colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colorBlending.attachmentCount = 1;
                colorBlending.pAttachments = &colorBlendAttachment;

                VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
                depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthStencilState.depthTestEnable = VK_FALSE;
                depthStencilState.depthWriteEnable = VK_FALSE;
                depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
                depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;

                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = (float) m_swapChain.getExtent().width;
                viewport.height = (float) m_swapChain.getExtent().height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                VkRect2D scissor{};
                scissor.offset = {0, 0};
                scissor.extent = m_swapChain.getExtent();

                VkPipelineViewportStateCreateInfo viewportState{};
                viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                viewportState.viewportCount = 1;
                viewportState.pViewports = &viewport;
                viewportState.scissorCount = 1;
                viewportState.pScissors = &scissor;

                const std::vector<VkDynamicState> dynamicStates = {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_SCISSOR,
                    VK_DYNAMIC_STATE_LINE_WIDTH
                };

                VkPipelineDynamicStateCreateInfo dynamicState{};
                dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                dynamicState.pDynamicStates = dynamicStates.data();
                dynamicState.dynamicStateCount = 3;
                dynamicState.flags = 0;

                VkPipelineMultisampleStateCreateInfo multisampling{};
                if (m_device.getMsaaSamples() > 1) {
                    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                    multisampling.sampleShadingEnable = VK_TRUE;
                    multisampling.minSampleShading = 0.2f;
                    multisampling.rasterizationSamples = m_device.getMsaaSamples();
                } else {
                    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                    multisampling.sampleShadingEnable = VK_FALSE;
                    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
                }

                VulkanShaderModule vertShaderModule = 
                    VulkanShaderModule(
                            "src/shaders/dynamicUniformBuffersVert.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_VERTEX_BIT);
                VulkanShaderModule fragShaderModule = 
                    VulkanShaderModule(
                            "src/shaders/dynamicUniformBuffersFrag.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_FRAGMENT_BIT);

                VkPipelineShaderStageCreateInfo shaderStages[] = {
                    vertShaderModule.getStageCreateInfo(), 
                    fragShaderModule.getStageCreateInfo()
                };

                auto bindingDescription = Vertex::getBindingDescription();
                auto attributeDescriptions = Vertex::getAttributeDescriptions();

                VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
                vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                vertexInputInfo.vertexBindingDescriptionCount = 1;
                vertexInputInfo.vertexAttributeDescriptionCount = 
                    static_cast<uint32_t>(attributeDescriptions.size());
                vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
                vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

                VkGraphicsPipelineCreateInfo pipelineInfo{};
                pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                pipelineInfo.stageCount = 2;
                pipelineInfo.pStages = shaderStages;
                pipelineInfo.pInputAssemblyState = &inputAssembly;
                pipelineInfo.pViewportState = &viewportState;
                pipelineInfo.pRasterizationState = &rasterizer;
                pipelineInfo.pMultisampleState = &multisampling;
                pipelineInfo.pDepthStencilState = &depthStencilState;
                pipelineInfo.pColorBlendState = &colorBlending;
                pipelineInfo.layout = m_pipelineLayout;
                pipelineInfo.renderPass = m_renderPass.getRenderPass();
                pipelineInfo.subpass = 0;
                pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
                pipelineInfo.pDynamicState = &dynamicState;
                pipelineInfo.pVertexInputState = &vertexInputInfo;
                
                VK_CHECK_RESULT(vkCreateGraphicsPipelines(
                            m_device.getLogicalDevice(), 
                            VK_NULL_HANDLE, 
                            1, 
                            &pipelineInfo, 
                            nullptr, 
                            &m_pipeline));

                vkDestroyShaderModule(
                        m_device.getLogicalDevice(), 
                        vertShaderModule.getModule(), 
                        nullptr);
                vkDestroyShaderModule(
                        m_device.getLogicalDevice(), 
                        fragShaderModule.getModule(), 
                        nullptr);
            }

            void createVertexBuffer() override {
                m_vertexBuffer = VulkanBuffer(m_device);
                m_vertexBuffer.createWithStagingBuffer(vertices, 
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            }

            void createIndexBuffer() override {
                m_indexBuffer = VulkanBuffer(m_device);
                m_indexBuffer.createWithStagingBuffer(indices,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            }

            void createCoordinateSystemUniformBuffers() override {
                VkDeviceSize bufferSize = sizeof(CoordinatesSystemUniformBufferObject);

                m_coordinateSystemUniformBuffers.resize(m_swapChain.getImages().size());

                for (size_t i = 0; i < m_swapChain.getImages().size(); i++) {
                    m_coordinateSystemUniformBuffers[i] = VulkanBuffer(m_device);
                    m_coordinateSystemUniformBuffers[i].createBuffer(
                            bufferSize, 
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                            *m_coordinateSystemUniformBuffers[i].getBufferPointer(), 
                            *m_coordinateSystemUniformBuffers[i].getBufferMemoryPointer()
                            );
                }
            }

            void createDynamicUniformBuffers() {
                size_t minUboAlignment = m_device.properties.limits.minUniformBufferOffsetAlignment;
                m_dynamicAlignment = sizeof(uboDataDynamic);
                if (minUboAlignment > 0) {
                    m_dynamicAlignment = (m_dynamicAlignment + minUboAlignment - 1)
                        & ~(minUboAlignment - 1);
                }

                size_t bufferSize = NUM_OBJ * m_dynamicAlignment;
                uboDataDynamic.model = (glm::mat4*)alignedAlloc(bufferSize, m_dynamicAlignment);
                assert(uboDataDynamic.model);

                std::cout << "minUniformBufferOffsetAlignment = " << minUboAlignment << std::endl;
                std::cout << "dynamicAlignment = " << m_dynamicAlignment << std::endl;

                m_dynUbos.resize(m_swapChain.getImages().size());

                for (size_t i = 0; i < m_swapChain.getImages().size(); i++) {
                    m_dynUbos[i] = VulkanBuffer(m_device);
                    m_dynUbos[i].createBuffer(
                            bufferSize,
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                            *m_dynUbos[i].getBufferPointer(),
                            *m_dynUbos[i].getBufferMemoryPointer()
                            );
                }

                std::default_random_engine rndEngine((unsigned)time(nullptr));
                std::normal_distribution<float> rndDist(-1.0f, 1.0f);
                for (uint32_t i = 0; i < NUM_OBJ; i++) {
                    m_rotations[i] = glm::vec3(rndDist(rndEngine), 
                            rndDist(rndEngine), rndDist(rndEngine)) * 2.0f * (float) M_PI;
                    m_rotationsSpeeds[i] = glm::vec3(rndDist(rndEngine), 
                            rndDist(rndEngine), rndDist(rndEngine));
                }
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

                std::array<VkClearValue, 2> clearValues{};
                clearValues[1].color = {0.0f, 0.0f, 0.0f, 1.0f};
                clearValues[0].depthStencil = {1.0f, 0};

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
                    renderPassInfo.renderPass = m_renderPass.getRenderPass(); 
                    renderPassInfo.framebuffer = m_framebuffers[i]; 
                    renderPassInfo.renderArea.offset = {0, 0};
                    renderPassInfo.renderArea.extent = m_swapChain.getExtent();

                    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
                    renderPassInfo.pClearValues = clearValues.data();

                    vkCmdBeginRenderPass(
                            m_commandBuffers[i].getCommandBuffer(), 
                            &renderPassInfo, 
                            VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdSetViewport(m_commandBuffers[i].getCommandBuffer(), 0, 1, &viewport);
                    vkCmdSetScissor(m_commandBuffers[i].getCommandBuffer(), 0, 1, &scissor);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipeline);

                    VkBuffer vertexBuffers[] = {m_vertexBuffer.getBuffer()};
                    VkDeviceSize offsets[] = {0};
                    vkCmdBindVertexBuffers(m_commandBuffers[i].getCommandBuffer(), 
                            0, 
                            1, 
                            vertexBuffers, 
                            offsets);
                    vkCmdBindIndexBuffer(
                            m_commandBuffers[i].getCommandBuffer(), 
                            m_indexBuffer.getBuffer(), 
                            0, 
                            VK_INDEX_TYPE_UINT32);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipeline); 

                    for (uint32_t j = 0; j < NUM_OBJ; j++) {
                        uint32_t dynamicOffset = j * static_cast<uint32_t>(m_dynamicAlignment);
                        vkCmdBindDescriptorSets(
                                m_commandBuffers[i].getCommandBuffer(), 
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_pipelineLayout,
                                0, 
                                1, 
                                &m_descriptorSets.getDescriptorSets()[i], 
                                1, 
                                &dynamicOffset);
                               
                        vkCmdDrawIndexed(
                                m_commandBuffers[i].getCommandBuffer(), 
                                static_cast<uint32_t>(indices.size()),
                                1, 
                                0, 
                                0, 
                                0);

                        drawUI(m_commandBuffers[i].getCommandBuffer());
                    }

                    vkCmdEndRenderPass(m_commandBuffers[i].getCommandBuffer());

                    if (vkEndCommandBuffer(m_commandBuffers[i].getCommandBuffer()) != VK_SUCCESS) {
                        throw std::runtime_error("Recording of a command buffer failed!");
                    }
                }
            }

            void createDescriptorSetLayout() override {
                m_descriptorSetLayout = VulkanDescriptorSetLayout(m_device);

                VkDescriptorSetLayoutBinding uboLayoutBinding{};
                uboLayoutBinding.binding = 0;
                uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uboLayoutBinding.descriptorCount = 1;
                uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                uboLayoutBinding.pImmutableSamplers = nullptr;

                VkDescriptorSetLayoutBinding dynUboLayoutBinding{};
                dynUboLayoutBinding.binding = 1;
                dynUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                dynUboLayoutBinding.descriptorCount = 1;
                dynUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                dynUboLayoutBinding.pImmutableSamplers = nullptr;
        
                std::vector<VkDescriptorSetLayoutBinding> bindings = 
                {uboLayoutBinding, dynUboLayoutBinding};

                m_descriptorSetLayout.create(bindings);
            }

            void createDescriptorPool() override {
                m_descriptorPool = VulkanDescriptorPool(m_device, m_swapChain);

                std::vector<VkDescriptorPoolSize> poolSizes = std::vector<VkDescriptorPoolSize>(2);
                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = static_cast<uint32_t>(
                        m_swapChain.getImages().size());

                poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                poolSizes[1].descriptorCount = static_cast<uint32_t>(
                        m_swapChain.getImages().size());

                m_descriptorPool.create(poolSizes);
            }

            void createDescriptorSets() override {
                std::vector<std::vector<VulkanBuffer>> ubos{
                    m_coordinateSystemUniformBuffers,
                    m_dynUbos
                };

                std::vector<VkDeviceSize> ubosSizes{
                    sizeof(CoordinatesSystemUniformBufferObject),
                        sizeof(uboDataDynamic)};

                m_descriptorSets = VulkanDescriptorSets(
                        m_device,
                        m_descriptorSetLayout, m_descriptorPool);

                m_descriptorSets.create(static_cast<uint32_t>(m_swapChain.getImages().size()));

                for (size_t i = 0; i < m_swapChain.getImages().size(); i++) {
                    std::vector<VkWriteDescriptorSet> descriptorWrites = 
                        std::vector<VkWriteDescriptorSet>(2);

                    std::vector<VkDescriptorBufferInfo> buffersInfo(ubos.size());
                    buffersInfo[0].offset = 0;
                    buffersInfo[0].buffer = ubos[0][i].getBuffer();
                    buffersInfo[0].range = ubosSizes[0];
                    buffersInfo[1].offset = 0;
                    buffersInfo[1].buffer = ubos[1][i].getBuffer();
                    buffersInfo[1].range = ubosSizes[1];

                    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrites[0].dstBinding = 0;
                    descriptorWrites[0].dstArrayElement = 0;
                    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    descriptorWrites[0].descriptorCount = 1;
                    descriptorWrites[0].pBufferInfo = &buffersInfo[0];

                    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrites[1].dstBinding = 1;
                    descriptorWrites[1].dstArrayElement = 0;
                    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                    descriptorWrites[1].descriptorCount = 1;
                    descriptorWrites[1].pBufferInfo = &buffersInfo[1];

                    m_descriptorSets.update(descriptorWrites, i);
                }
            }

            void updateUniformBuffers(uint32_t currentImage) {
                CoordinatesSystemUniformBufferObject ubo{};

                ubo.model = glm::mat4(1.0f);

                ubo.view = m_camera.getViewMatrix();

                ubo.proj = glm::perspective(glm::radians(m_camera.getZoom()), 
                        m_swapChain.getExtent().width / (float) m_swapChain.getExtent().height, 
                        0.1f,  100.0f);

                ubo.proj[1][1] *= -1;

                ubo.camPos = m_camera.position();

                m_coordinateSystemUniformBuffers[currentImage].map();
                memcpy(m_coordinateSystemUniformBuffers[currentImage].getMappedMemory(), 
                        &ubo, sizeof(ubo));
                m_coordinateSystemUniformBuffers[currentImage].unmap();
            }

            void updateDynUbos(uint32_t currentImage) {
                uint32_t dim = static_cast<uint32_t>(pow(NUM_OBJ, (1.0f / 3.0f)));
                glm::vec3 offset(5.0f);

                for (uint32_t x = 0; x < dim; x++) {
                    for (uint32_t y = 0; y < dim; y++) {
                        for (uint32_t z = 0; z < dim; z++) {
                            uint32_t index = x * dim * dim + y * dim + z;

                            // Aligned offset
                            glm::mat4* modelMat = (glm::mat4*)(((uint64_t)uboDataDynamic.model + (index * m_dynamicAlignment)));
                            *modelMat = glm::mat4(1.0f);
                            
                            /* // Update Rotation */
                            m_rotations[index] += m_rotationsSpeeds[index] * 0.05f;

                            /* //Update matrices */
                            glm::vec3 pos = glm::vec3(
                                    -((dim * offset.x) / 2.0f) + offset.x / 2.0f + x * offset.x, 
                                    -((dim * offset.y) / 2.0f) + offset.y / 2.0f + y * offset.y,
                                    -((dim * offset.z) / 2.0f) + offset.z / 2.0f + z * offset.z);

                            *modelMat = glm::translate(glm::mat4(1.0f), pos);
                            *modelMat = glm::rotate(*modelMat, m_rotations[index].x, glm::vec3(1.0f, 1.0f, 0.0f));
                            *modelMat = glm::rotate(*modelMat, m_rotations[index].y, glm::vec3(0.0f, 1.0f, 0.0f));
                            *modelMat = glm::rotate(*modelMat, m_rotations[index].z, glm::vec3(0.0f, 0.0f, 1.0f));
                        }
                    }
                }

                m_dynUbos[currentImage].map();
                memcpy(m_dynUbos[currentImage].getMappedMemory(), 
                        uboDataDynamic.model, 
                        m_dynUbos[currentImage].getSize());
                m_dynUbos[currentImage].unmap();
            }
                        
            void OnUpdateUI (UI *ui) override {
            }
    };

}

VULKAN_EXAMPLE_MAIN()
