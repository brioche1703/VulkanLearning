#include "VulkanBase.hpp"

namespace VulkanLearning {

    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, -0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.0f,  0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}},
    };

    const std::vector<uint32_t> indices = {
        0, 1, 2, 0
    };

    class VulkanExample : public VulkanBase {
        private:
            VulkanDescriptorSets m_descriptorSets;

            VkPipeline m_pipeline;

            VkPipelineLayout m_pipelineLayout;

        public:
            VulkanExample() {}

            ~VulkanExample() {
                cleanupSwapChain();

                m_descriptorSetLayout.cleanup();

                m_vertexBuffer.cleanup();
                m_indexBuffer.cleanup();
            }

            void run() {
                VulkanBase::run();
            }
        private:

            void initVulkan() override {
                VulkanBase::initVulkan();

                m_window.setTitle("Simple triangle");
                m_camera.setPosition(glm::vec3(0.0f, 0.0f, 3.0f));

                createVertexBuffer();
                createIndexBuffer();

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
                uint32_t imageIndex;
                VulkanBase::acquireFrame(&imageIndex);
                updateUniformBuffers(imageIndex);
                VK_CHECK_RESULT(vkQueueSubmit(m_device.getGraphicsQueue(), 1, &m_submitInfo, m_syncObjects.getInFlightFences()[m_currentFrame]));
                VulkanBase::presentFrame(imageIndex);
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

                VkAttachmentReference colorAttachmentRef{};
                colorAttachmentRef.attachment = 0;
                colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentDescription depthAttachment{};
                depthAttachment.format = m_device.findDepthFormat();
                depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentReference depthAttachmentRef{};
                depthAttachmentRef.attachment = 1;
                depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments = &colorAttachmentRef;

                const std::vector<VkAttachmentDescription> attachments = 
                    { colorAttachment, depthAttachment};

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
                            "src/shaders/simpleTriangleShaderVert.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_VERTEX_BIT);
                VulkanShaderModule fragShaderModule = 
                    VulkanShaderModule(
                            "src/shaders/simpleTriangleShaderFrag.spv", 
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
                    m_coordinateSystemUniformBuffers[i].createBuffer(bufferSize, 
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                            *m_coordinateSystemUniformBuffers[i].getBufferPointer(), 
                            *m_coordinateSystemUniformBuffers[i].getBufferMemoryPointer());
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
                clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
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

                    vkCmdBeginRenderPass(m_commandBuffers[i].getCommandBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                    
                    vkCmdSetViewport(m_commandBuffers[i].getCommandBuffer(), 0, 1, &viewport);
                    vkCmdSetScissor(m_commandBuffers[i].getCommandBuffer(), 0, 1, &scissor);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipeline);

                    VkBuffer vertexBuffers[] = {m_vertexBuffer.getBuffer()};
                    VkDeviceSize offsets[] = {0};
                    vkCmdBindVertexBuffers(
                            m_commandBuffers[i].getCommandBuffer(), 
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

                    vkCmdBindDescriptorSets(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelineLayout,
                            0, 
                            1, 
                            &m_descriptorSets.getDescriptorSets()[i], 
                            0, 
                            nullptr);

                    vkCmdDrawIndexed(
                            m_commandBuffers[i].getCommandBuffer(), 
                            static_cast<uint32_t>(indices.size()),
                            1, 
                            0, 
                            0, 
                            0);

                    drawUI(m_commandBuffers[i].getCommandBuffer());

                    vkCmdEndRenderPass(m_commandBuffers[i].getCommandBuffer());

                    VK_CHECK_RESULT(vkEndCommandBuffer(m_commandBuffers[i].getCommandBuffer()));
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
        
                std::vector<VkDescriptorSetLayoutBinding> bindings = 
                {uboLayoutBinding};

                m_descriptorSetLayout.create(bindings);
            }

            void createDescriptorPool() override {
                m_descriptorPool = VulkanDescriptorPool(m_device, m_swapChain);

                std::vector<VkDescriptorPoolSize> poolSizes = std::vector<VkDescriptorPoolSize>(1);
                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = static_cast<uint32_t>(
                        m_swapChain.getImages().size());

                m_descriptorPool.create(poolSizes);
            }

            void createDescriptorSets() override {
                std::vector<std::vector<VulkanBuffer>> ubos {
                    m_coordinateSystemUniformBuffers
                };
                std::vector<VkDeviceSize> ubosSizes{
                    sizeof(CoordinatesSystemUniformBufferObject)
                };

                m_descriptorSets = VulkanDescriptorSets(
                        m_device, 
                        m_descriptorSetLayout, 
                        m_descriptorPool);

                m_descriptorSets.create(static_cast<uint32_t>(m_swapChain.getImages().size()));

                for (size_t i = 0; i < m_swapChain.getImages().size(); i++) {
                    VkDescriptorBufferInfo bufferInfo{};
                    bufferInfo.offset = 0;
                    bufferInfo.buffer = ubos[0][i].getBuffer();
                    bufferInfo.range = ubosSizes[0];

                    std::vector<VkWriteDescriptorSet> descriptorWrites = 
                        std::vector<VkWriteDescriptorSet>(1);

                    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrites[0].dstBinding = 0;
                    descriptorWrites[0].dstArrayElement = 0;
                    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    descriptorWrites[0].descriptorCount = 1;
                    descriptorWrites[0].pBufferInfo = &bufferInfo;

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

            void OnUpdateUI (UI *ui) override {
            }
    };

}

VULKAN_EXAMPLE_MAIN()
