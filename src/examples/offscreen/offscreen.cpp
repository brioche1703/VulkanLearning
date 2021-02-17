#define TINYGLTF_IMPLEMENTATION

#include "VulkanBase.hpp"

#include "VulkanglTFModel.hpp"


namespace VulkanLearning {

#define NUM_LIGHTS 64

    class VulkanExample : public VulkanBase {

        private:
            bool debugDisplay = false;

            VulkanTexture2D m_glassTexture;

            struct {
                VulkanglTFModel model;
                VulkanglTFModel plane;
            } m_models;

            struct UBOMatrices {
                glm::mat4 projection;
                glm::mat4 model;
                glm::mat4 view;
                glm::vec4 lightPos = glm::vec4(0.0f, 0.0f, 0.0, 1.0f);
            } m_uboMatrices;

            struct {
                VulkanBuffer uboShared;
                VulkanBuffer uboMirror;
                VulkanBuffer uboOffscreen;
            } m_ubos;

            struct {
                VkPipeline shaded;
                VkPipeline shadedOffscreen;
                VkPipeline mirror;
            } m_pipelines;

            struct {
                VkPipelineLayout textured;
                VkPipelineLayout shaded;
            } m_pipelineLayouts;

            struct {
                VkDescriptorSet offscreen;
                VkDescriptorSet mirror;
                VkDescriptorSet model;
            } m_descriptorSets;

            struct {
                VulkanDescriptorSetLayout textured;
                VulkanDescriptorSetLayout shaded;
            } m_descriptorSetLayouts;

            struct FramebufferAttachment {
                VkImage image;
                VkDeviceMemory memory;
                VkImageView view;
                VkFormat format;
            };

            struct OffscreenPass {
                int32_t width;
                int32_t height;
                VkFramebuffer framebuffer;
                FramebufferAttachment color, depth;
                VkRenderPass renderPass;
                VkSampler sampler;
                VkDescriptorImageInfo descriptor;
            } m_offscreenPass;

            glm::vec3 m_modelPosition = glm::vec3(0.0f, -1.0f, 0.0f);
            glm::vec3 m_modelRotation = glm::vec3(0.0f);

        public:
            VulkanExample() {}

            ~VulkanExample() {
                cleanupSwapChain();

                m_ubos.uboMirror.cleanup();
                m_ubos.uboShared.cleanup();
                m_ubos.uboOffscreen.cleanup();

                m_descriptorSetLayout.cleanup();

                m_models.model.~VulkanglTFModel();
                m_models.plane.~VulkanglTFModel();
            }

            void run() {
                VulkanBase::run();
            }

        private:

            void checkAndEnableFeatures() override {
                if (m_device.features.shaderClipDistance) {
                    m_device.enabledFeatures.shaderClipDistance = VK_TRUE;
                }
            }

            void initVulkan() override {
                m_msaaSamples = 1;
                VulkanBase::initVulkan();

                m_window.setTitle("Offscreen");
                m_camera.setPosition(glm::vec3(0.0f, 1.0f, 5.0f));

                createOffscreenPass();

                loadAssets();

                createDescriptorSetLayout();
                createPipelines();

                createUniformBuffers();

                createDescriptorPool();
                createDescriptorSets();
                createCommandBuffers();
            }

            void drawFrame() override {
                uint32_t imageIndex;
                VulkanBase::acquireFrame(&imageIndex);
                updateUniformBuffers();
                VK_CHECK_RESULT(vkQueueSubmit(m_device.getGraphicsQueue(), 1, &m_submitInfo, m_syncObjects.getInFlightFences()[m_currentFrame]));
                VulkanBase::presentFrame(imageIndex);
            }

            void createPipelines() override {
                VkPipelineLayoutCreateInfo pipelineLayoutShadedCI = {};
                pipelineLayoutShadedCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutShadedCI.setLayoutCount = 1;
                pipelineLayoutShadedCI.pSetLayouts = m_descriptorSetLayouts.shaded.getDescriptorSetLayoutPointer();

                vkCreatePipelineLayout(
                        m_device.getLogicalDevice(), 
                        &pipelineLayoutShadedCI, 
                        nullptr, 
                        &m_pipelineLayouts.shaded);

                VkPipelineLayoutCreateInfo pipelineLayoutTexturedCI{};
                pipelineLayoutTexturedCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutTexturedCI.setLayoutCount = 1;
                pipelineLayoutTexturedCI.pSetLayouts = m_descriptorSetLayouts.textured.getDescriptorSetLayoutPointer();

                vkCreatePipelineLayout(
                        m_device.getLogicalDevice(), 
                        &pipelineLayoutTexturedCI, 
                        nullptr, 
                        &m_pipelineLayouts.textured);

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
                depthStencilState.depthTestEnable = VK_TRUE;
                depthStencilState.depthWriteEnable = VK_TRUE;
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

                VkGraphicsPipelineCreateInfo pipelineInfo{};
                pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                pipelineInfo.stageCount = 2;
                pipelineInfo.pInputAssemblyState = &inputAssembly;
                pipelineInfo.pViewportState = &viewportState;
                pipelineInfo.pRasterizationState = &rasterizer;
                pipelineInfo.pMultisampleState = &multisampling;
                pipelineInfo.pDepthStencilState = &depthStencilState;
                pipelineInfo.pColorBlendState = &colorBlending;
                pipelineInfo.renderPass = m_renderPass.getRenderPass();
                pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
                pipelineInfo.pDynamicState = &dynamicState;

                // Mirror pipeline
                std::vector<VulkanShaderModule> shaderModulesMirror {
                    VulkanShaderModule(
                            "src/shaders/offscreen/mirrorVert.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_VERTEX_BIT),
                        VulkanShaderModule(
                                "src/shaders/offscreen/mirrorFrag.spv", 
                                &m_device, 
                                VK_SHADER_STAGE_FRAGMENT_BIT)
                };

                VkPipelineShaderStageCreateInfo shaderStages[] = {
                    shaderModulesMirror[0].getStageCreateInfo(), 
                    shaderModulesMirror[1].getStageCreateInfo()
                };

                pipelineInfo.subpass = 0;
                pipelineInfo.layout = m_pipelineLayouts.textured;
                pipelineInfo.pStages = shaderStages;
                pipelineInfo.pVertexInputState = Vertex::getPipelineVertexInputState({
                        VertexComponent::Position,
                        VertexComponent::Color,
                        VertexComponent::Normal});

                rasterizer.cullMode = VK_CULL_MODE_NONE;

                VK_CHECK_RESULT(vkCreateGraphicsPipelines(
                            m_device.getLogicalDevice(), 
                            VK_NULL_HANDLE, 
                            1, 
                            &pipelineInfo, 
                            nullptr, 
                            &m_pipelines.mirror));

                // Phong pipeline
                std::vector<VulkanShaderModule> shaderModulesPhong {
                    VulkanShaderModule(
                            "src/shaders/offscreen/phongVert.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_VERTEX_BIT),
                        VulkanShaderModule(
                                "src/shaders/offscreen/phongFrag.spv", 
                                &m_device, 
                                VK_SHADER_STAGE_FRAGMENT_BIT)
                };


                shaderStages[0] = shaderModulesPhong[0].getStageCreateInfo();
                shaderStages[1] = shaderModulesPhong[1].getStageCreateInfo();

                pipelineInfo.layout = m_pipelineLayouts.shaded;

                rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;

                VK_CHECK_RESULT(vkCreateGraphicsPipelines(
                            m_device.getLogicalDevice(), 
                            VK_NULL_HANDLE, 
                            1, 
                            &pipelineInfo, 
                            nullptr, 
                            &m_pipelines.shaded));


                // Offscreen pipeline
                pipelineInfo.renderPass = m_offscreenPass.renderPass;
                rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;

                VK_CHECK_RESULT(vkCreateGraphicsPipelines(
                            m_device.getLogicalDevice(), 
                            VK_NULL_HANDLE, 
                            1, 
                            &pipelineInfo, 
                            nullptr, 
                            &m_pipelines.shadedOffscreen));

                shaderModulesMirror[0].cleanup(&m_device);
                shaderModulesMirror[1].cleanup(&m_device);
                shaderModulesPhong[0].cleanup(&m_device);
                shaderModulesPhong[1].cleanup(&m_device);
            }

            void createUniformBuffers() {
                m_ubos.uboMirror = VulkanBuffer(m_device);
                m_ubos.uboOffscreen = VulkanBuffer(m_device);
                m_ubos.uboShared = VulkanBuffer(m_device);

                m_ubos.uboMirror.createBuffer(
                        sizeof(m_uboMatrices), 
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                m_ubos.uboOffscreen.createBuffer(
                        sizeof(m_uboMatrices), 
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                m_ubos.uboShared.createBuffer(
                        sizeof(m_uboMatrices), 
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                m_ubos.uboMirror.map();
                m_ubos.uboOffscreen.map();
                m_ubos.uboShared.map();

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

                VkRenderPassBeginInfo renderPassBeginInfo = {};
                renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.renderArea.offset.x = 0;
                renderPassBeginInfo.renderArea.offset.y = 0;
                renderPassBeginInfo.clearValueCount = 2;
                renderPassBeginInfo.pClearValues = clearValues;

                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = 0;
                beginInfo.pInheritanceInfo = nullptr;

                for (int32_t i = 0; i < m_commandBuffers.size(); ++i)
                {
                    VK_CHECK_RESULT(vkBeginCommandBuffer(m_commandBuffers[i].getCommandBuffer(), &beginInfo));

                    // 1rst render pass offscreen
                    renderPassBeginInfo.framebuffer = m_offscreenPass.framebuffer;
                    renderPassBeginInfo.renderPass = m_offscreenPass.renderPass;
                    renderPassBeginInfo.renderArea.extent.width = m_offscreenPass.width;
                    renderPassBeginInfo.renderArea.extent.height = m_offscreenPass.height;

                    vkCmdBeginRenderPass(m_commandBuffers[i].getCommandBuffer(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                    VkViewport viewport = {};
                    viewport.width = m_offscreenPass.width;
                    viewport.height = m_offscreenPass.height;
                    viewport.minDepth = 0.0f;
                    viewport.maxDepth = 1.0f;

                    VkRect2D scissor = {};
                    scissor.extent.width = m_offscreenPass.width;
                    scissor.extent.height = m_offscreenPass.height;
                    scissor.offset.x = 0;
                    scissor.offset.y = 0;

                    vkCmdSetViewport(m_commandBuffers[i].getCommandBuffer(), 0, 1, &viewport);
                    vkCmdSetScissor(m_commandBuffers[i].getCommandBuffer(), 0, 1, &scissor);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelines.shadedOffscreen);

                    vkCmdBindDescriptorSets(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelineLayouts.shaded,
                            0, 
                            1, 
                            &m_descriptorSets.offscreen,
                            0, 
                            nullptr);

                    m_models.model.draw(m_commandBuffers[i].getCommandBuffer());
                    vkCmdEndRenderPass(m_commandBuffers[i].getCommandBuffer());

                    // 2nd render pass
                    renderPassBeginInfo.framebuffer = m_framebuffers[i];
                    renderPassBeginInfo.renderPass = m_renderPass.getRenderPass(); 
                    renderPassBeginInfo.renderArea.extent.width = m_swapChain.getExtent().width;
                    renderPassBeginInfo.renderArea.extent.height = m_swapChain.getExtent().height;

                    viewport.width = m_swapChain.getExtent().width;
                    viewport.height = m_swapChain.getExtent().height;
                    viewport.minDepth = 0.0f;
                    viewport.maxDepth = 1.0f;

                    scissor.extent.width = m_swapChain.getExtent().width;
                    scissor.extent.height = m_swapChain.getExtent().height;
                    scissor.offset.x = 0;
                    scissor.offset.y = 0;

                    vkCmdBeginRenderPass(
                            m_commandBuffers[i].getCommandBuffer(), 
                            &renderPassBeginInfo, 
                            VK_SUBPASS_CONTENTS_INLINE);

                    // Render reflexion plane
                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelines.mirror);

                    vkCmdBindDescriptorSets(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelineLayouts.textured,
                            0, 
                            1, 
                            &m_descriptorSets.mirror,
                            0, 
                            nullptr);

                    m_models.plane.draw(m_commandBuffers[i].getCommandBuffer());

                    // Render model 
                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelines.shaded);

                    vkCmdBindDescriptorSets(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelineLayouts.shaded,
                            0, 
                            1, 
                            &m_descriptorSets.model,
                            0, 
                            nullptr);

                    m_models.model.draw(m_commandBuffers[i].getCommandBuffer());


                    drawUI(m_commandBuffers[i].getCommandBuffer());

                    vkCmdEndRenderPass(m_commandBuffers[i].getCommandBuffer());
                    VK_CHECK_RESULT(vkEndCommandBuffer(m_commandBuffers[i].getCommandBuffer()));
                }
            }

            void createDescriptorSetLayout() override {
                m_descriptorSetLayouts.shaded = VulkanDescriptorSetLayout(m_device);
                m_descriptorSetLayouts.textured = VulkanDescriptorSetLayout(m_device);

                std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindingsShaded(1);
                descriptorSetLayoutBindingsShaded[0].binding = 0;
                descriptorSetLayoutBindingsShaded[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorSetLayoutBindingsShaded[0].descriptorCount = 1;
                descriptorSetLayoutBindingsShaded[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                descriptorSetLayoutBindingsShaded[0].pImmutableSamplers = nullptr;

                std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindingsTextured(3);
                descriptorSetLayoutBindingsTextured[0].binding = 0;
                descriptorSetLayoutBindingsTextured[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorSetLayoutBindingsTextured[0].descriptorCount = 1;
                descriptorSetLayoutBindingsTextured[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                descriptorSetLayoutBindingsTextured[0].pImmutableSamplers = nullptr;

                descriptorSetLayoutBindingsTextured[1].binding = 1;
                descriptorSetLayoutBindingsTextured[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorSetLayoutBindingsTextured[1].descriptorCount = 1;
                descriptorSetLayoutBindingsTextured[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                descriptorSetLayoutBindingsTextured[1].pImmutableSamplers = nullptr;

                descriptorSetLayoutBindingsTextured[2].binding = 2;
                descriptorSetLayoutBindingsTextured[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorSetLayoutBindingsTextured[2].descriptorCount = 1;
                descriptorSetLayoutBindingsTextured[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                descriptorSetLayoutBindingsTextured[2].pImmutableSamplers = nullptr;

                m_descriptorSetLayouts.shaded.create(descriptorSetLayoutBindingsShaded);
                m_descriptorSetLayouts.textured.create(descriptorSetLayoutBindingsTextured);
            }

            void createDescriptorPool() override {
                m_descriptorPool = VulkanDescriptorPool(m_device, m_swapChain);

                std::vector<VkDescriptorPoolSize> poolSizes = std::vector<VkDescriptorPoolSize>(2);

                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = 6;
                poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[1].descriptorCount = 8;

                m_descriptorPool.create(poolSizes, 5);
            }

            void createDescriptorSets() override {
                // Mirror
                VkDescriptorSetAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool = m_descriptorPool.getDescriptorPool();
                allocInfo.descriptorSetCount = 1;
                allocInfo.pSetLayouts = m_descriptorSetLayouts.textured.getDescriptorSetLayoutPointer();

                VK_CHECK_RESULT(vkAllocateDescriptorSets(
                            m_device.getLogicalDevice(), 
                            &allocInfo, 
                            &m_descriptorSets.mirror));

                std::vector<VkWriteDescriptorSet> descriptorMirror(2);
                descriptorMirror[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorMirror[0].dstBinding = 0;
                descriptorMirror[0].dstArrayElement = 0;
                descriptorMirror[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorMirror[0].descriptorCount = 1;
                descriptorMirror[0].pBufferInfo = m_ubos.uboMirror.getDescriptorPointer();
                descriptorMirror[0].dstSet = m_descriptorSets.mirror;

                descriptorMirror[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorMirror[1].dstBinding = 1;
                descriptorMirror[1].dstArrayElement = 0;
                descriptorMirror[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorMirror[1].descriptorCount = 1;
                descriptorMirror[1].pImageInfo = &m_offscreenPass.descriptor;
                descriptorMirror[1].dstSet = m_descriptorSets.mirror;

                vkUpdateDescriptorSets(
                        m_device.getLogicalDevice(), 
                        static_cast<uint32_t>(descriptorMirror.size()), 
                        descriptorMirror.data(), 
                        0, 
                        NULL);

                // Model
                allocInfo.pSetLayouts = m_descriptorSetLayouts.shaded.getDescriptorSetLayoutPointer();

                VK_CHECK_RESULT(vkAllocateDescriptorSets(
                            m_device.getLogicalDevice(), 
                            &allocInfo, 
                            &m_descriptorSets.model));

                std::vector<VkWriteDescriptorSet> descriptorModel(1);
                descriptorModel[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorModel[0].dstBinding = 0;
                descriptorModel[0].dstArrayElement = 0;
                descriptorModel[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorModel[0].descriptorCount = 1;
                descriptorModel[0].pBufferInfo = m_ubos.uboShared.getDescriptorPointer();
                descriptorModel[0].dstSet = m_descriptorSets.model;

                vkUpdateDescriptorSets(
                        m_device.getLogicalDevice(), 
                        static_cast<uint32_t>(descriptorModel.size()), 
                        descriptorModel.data(), 
                        0, 
                        NULL);

                // Offscreen
                VK_CHECK_RESULT(vkAllocateDescriptorSets(
                            m_device.getLogicalDevice(), 
                            &allocInfo, 
                            &m_descriptorSets.offscreen));

                std::vector<VkWriteDescriptorSet> descriptorOffscreen(1);
                descriptorOffscreen[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorOffscreen[0].dstBinding = 0;
                descriptorOffscreen[0].dstArrayElement = 0;
                descriptorOffscreen[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorOffscreen[0].descriptorCount = 1;
                descriptorOffscreen[0].pBufferInfo = m_ubos.uboOffscreen.getDescriptorPointer();
                descriptorOffscreen[0].dstSet = m_descriptorSets.offscreen;

                vkUpdateDescriptorSets(
                        m_device.getLogicalDevice(), 
                        static_cast<uint32_t>(descriptorOffscreen.size()), 
                        descriptorOffscreen.data(), 
                        0, 
                        NULL);
            }

            void updateUniformBuffers() {
                m_uboMatrices.projection = glm::perspective(
                        glm::radians(m_camera.getZoom()), 
                        m_swapChain.getExtent().width / (float) m_swapChain.getExtent().height, 
                        0.1f,  100.0f);

                m_uboMatrices.projection[1][1] *= -1;
                m_uboMatrices.view = m_camera.getViewMatrix();

                m_uboMatrices.model = glm::mat4(1.0f);
                m_uboMatrices.model = glm::translate(m_uboMatrices.model, glm::vec3(0.0f, 1.0f, 0.0f));
                memcpy(m_ubos.uboShared.getMappedMemory(), &m_uboMatrices, sizeof(m_uboMatrices));

                m_uboMatrices.model = glm::mat4(1.0f);
                memcpy(m_ubos.uboMirror.getMappedMemory(), &m_uboMatrices, sizeof(m_uboMatrices));

                m_uboMatrices.model = glm::scale(m_uboMatrices.model, glm::vec3(1.0f, -1.0f, 1.0f));
                m_uboMatrices.model = glm::translate(m_uboMatrices.model, glm::vec3(0.0f, 1.0f, 0.0f));
                memcpy(m_ubos.uboOffscreen.getMappedMemory(), &m_uboMatrices, sizeof(m_uboMatrices));
            }

            void loadAssets() {
                uint32_t glTFLoadingFlags = FileLoadingFlags::PreTransformVertices | FileLoadingFlags::PreMultiplyVertexColors;
                m_models.model.loadFromFile("src/models/chinesedragon.gltf", &m_device, m_device.getGraphicsQueue(), glTFLoadingFlags);
                m_models.plane.loadFromFile("src/models/plane.gltf", &m_device, m_device.getGraphicsQueue(), glTFLoadingFlags);
            }

            void OnUpdateUI (UI *ui) override {
                if (ui->header("Settings")) {
                }
            }

            void clearAttachment(FramebufferAttachment *attachment) {
                vkDestroyImageView(m_device.getLogicalDevice(), attachment->view, nullptr);
                vkDestroyImage(m_device.getLogicalDevice(), attachment->image, nullptr);
                vkFreeMemory(m_device.getLogicalDevice(), attachment->memory, nullptr);
            }

            void createAttachment(VkFormat format, VkImageUsageFlags usage, FramebufferAttachment *attachment) {
                if (attachment->image != VK_NULL_HANDLE) {
                    clearAttachment(attachment);
                }

                VkImageAspectFlags aspectMask = 0;
                VkImageLayout imageLayout;

                attachment->format = format;

                if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
                    aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                    aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                    imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }

                VkImageCreateInfo imageCreateInfo = {};
                imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
                imageCreateInfo.format = format;
                imageCreateInfo.extent.width = m_swapChain.getExtent().width;
                imageCreateInfo.extent.height = m_swapChain.getExtent().height;
                imageCreateInfo.extent.depth = 1;
                imageCreateInfo.mipLevels = 1;
                imageCreateInfo.arrayLayers = 1;
                imageCreateInfo.samples = m_device.getMsaaSamples();
                imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageCreateInfo.usage = usage | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

                VK_CHECK_RESULT(vkCreateImage(
                            m_device.getLogicalDevice(), 
                            &imageCreateInfo, 
                            nullptr, 
                            &attachment->image));

                VkMemoryAllocateInfo memAllocInfo = {};
                memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                VkMemoryRequirements memReqs;
                vkGetImageMemoryRequirements(
                        m_device.getLogicalDevice(), 
                        attachment->image, 
                        &memReqs);
                memAllocInfo.allocationSize = memReqs.size;
                memAllocInfo.memoryTypeIndex = m_device.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                VK_CHECK_RESULT(vkAllocateMemory(
                            m_device.getLogicalDevice(), 
                            &memAllocInfo, 
                            nullptr, 
                            &attachment->memory));

                VK_CHECK_RESULT(vkBindImageMemory(
                            m_device.getLogicalDevice(), 
                            attachment->image, 
                            attachment->memory, 
                            0));

                VkImageViewCreateInfo imageViewCreateInfo = {};
                imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                imageViewCreateInfo.format = format;
                imageViewCreateInfo.subresourceRange = {};
                imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
                imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
                imageViewCreateInfo.subresourceRange.levelCount = 1;
                imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
                imageViewCreateInfo.subresourceRange.layerCount = 1;
                imageViewCreateInfo.image = attachment->image;

                VK_CHECK_RESULT(vkCreateImageView(
                            m_device.getLogicalDevice(), 
                            &imageViewCreateInfo, 
                            nullptr, 
                            &attachment->view));
            }

            void createOffscreenPass() {
                m_offscreenPass.width = m_swapChain.getExtent().width;
                m_offscreenPass.height = m_swapChain.getExtent().height;

                VkImageCreateInfo image = {};
                image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                image.imageType = VK_IMAGE_TYPE_2D;
                image.format = m_swapChain.getImageFormat();
                image.extent.width = m_offscreenPass.width;
                image.extent.height = m_offscreenPass.height;
                image.extent.depth = 1;
                image.mipLevels = 1;
                image.arrayLayers = 1;
                image.samples = VK_SAMPLE_COUNT_1_BIT;
                image.tiling = VK_IMAGE_TILING_OPTIMAL;
                image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

                VK_CHECK_RESULT(vkCreateImage(
                            m_device.getLogicalDevice(), 
                            &image, 
                            nullptr, 
                            &m_offscreenPass.color.image));

                VkMemoryAllocateInfo memAlloc = {};
                memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                VkMemoryRequirements memReqs;

                vkGetImageMemoryRequirements(
                        m_device.getLogicalDevice(), 
                        m_offscreenPass.color.image, 
                        &memReqs);
                memAlloc.allocationSize = memReqs.size;
                memAlloc.memoryTypeIndex = m_device.findMemoryType(
                        memReqs.memoryTypeBits, 
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                VK_CHECK_RESULT(vkAllocateMemory(
                            m_device.getLogicalDevice(), 
                            &memAlloc, 
                            nullptr, 
                            &m_offscreenPass.color.memory));
                VK_CHECK_RESULT(vkBindImageMemory(
                            m_device.getLogicalDevice(), 
                            m_offscreenPass.color.image, 
                            m_offscreenPass.color.memory, 
                            0));

                VkImageViewCreateInfo colorImageView = {};
                colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
                colorImageView.format = m_swapChain.getImageFormat();
                colorImageView.subresourceRange = {};
                colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                colorImageView.subresourceRange.baseMipLevel = 0;
                colorImageView.subresourceRange.levelCount = 1;
                colorImageView.subresourceRange.baseArrayLayer = 0;
                colorImageView.subresourceRange.layerCount = 1;
                colorImageView.image = m_offscreenPass.color.image;
                VK_CHECK_RESULT(vkCreateImageView(
                            m_device.getLogicalDevice(), 
                            &colorImageView, 
                            nullptr, 
                            &m_offscreenPass.color.view));

                VkSamplerCreateInfo samplerInfo = {};
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerInfo.magFilter = VK_FILTER_LINEAR;
                samplerInfo.minFilter = VK_FILTER_LINEAR;
                samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeV = samplerInfo.addressModeU;
                samplerInfo.addressModeW = samplerInfo.addressModeU;
                samplerInfo.mipLodBias = 0.0f;
                samplerInfo.maxAnisotropy = 1.0f;
                samplerInfo.minLod = 0.0f;
                samplerInfo.maxLod = 1.0f;
                samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

                VK_CHECK_RESULT(vkCreateSampler(
                            m_device.getLogicalDevice(), 
                            &samplerInfo, nullptr, 
                            &m_offscreenPass.sampler));

                image.format = m_device.findDepthFormat();
                image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

                VK_CHECK_RESULT(vkCreateImage(
                            m_device.getLogicalDevice(), 
                            &image, 
                            nullptr, 
                            &m_offscreenPass.depth.image));
                vkGetImageMemoryRequirements(
                        m_device.getLogicalDevice(), 
                        m_offscreenPass.depth.image, 
                        &memReqs);

                memAlloc.allocationSize = memReqs.size;
                memAlloc.memoryTypeIndex = m_device.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                VK_CHECK_RESULT(vkAllocateMemory(
                            m_device.getLogicalDevice(), 
                            &memAlloc, 
                            nullptr, 
                            &m_offscreenPass.depth.memory));

                VK_CHECK_RESULT(vkBindImageMemory(
                            m_device.getLogicalDevice(), 
                            m_offscreenPass.depth.image, 
                            m_offscreenPass.depth.memory, 0));

                VkImageViewCreateInfo depthStencilView = {};
                depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
                depthStencilView.format = m_device.findDepthFormat();
                depthStencilView.flags = 0;
                depthStencilView.subresourceRange = {};
                depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                depthStencilView.subresourceRange.baseMipLevel = 0;
                depthStencilView.subresourceRange.levelCount = 1;
                depthStencilView.subresourceRange.baseArrayLayer = 0;
                depthStencilView.subresourceRange.layerCount = 1;
                depthStencilView.image = m_offscreenPass.depth.image;

                VK_CHECK_RESULT(vkCreateImageView(
                            m_device.getLogicalDevice(), 
                            &depthStencilView, 
                            nullptr, 
                            &m_offscreenPass.depth.view));

                std::vector<VkAttachmentDescription> attchmentDescriptions(2);
                attchmentDescriptions[0].format = m_swapChain.getImageFormat();
                attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
                attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                attchmentDescriptions[1].format = m_device.findDepthFormat();
                attchmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
                attchmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attchmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attchmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attchmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attchmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attchmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
                VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

                VkSubpassDescription subpassDescription = {};
                subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpassDescription.colorAttachmentCount = 1;
                subpassDescription.pColorAttachments = &colorReference;
                subpassDescription.pDepthStencilAttachment = &depthReference;

                // Use subpass dependencies for layout transitions
                std::vector<VkSubpassDependency> dependencies(2);

                dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[0].dstSubpass = 0;
                dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                dependencies[1].srcSubpass = 0;
                dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                VkRenderPassCreateInfo renderPassInfo = {};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
                renderPassInfo.pAttachments = attchmentDescriptions.data();
                renderPassInfo.subpassCount = 1;
                renderPassInfo.pSubpasses = &subpassDescription;
                renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
                renderPassInfo.pDependencies = dependencies.data();

                VK_CHECK_RESULT(vkCreateRenderPass(m_device.getLogicalDevice(), &renderPassInfo, nullptr, &m_offscreenPass.renderPass));

                VkImageView attachments[2];
                attachments[0] = m_offscreenPass.color.view;
                attachments[1] = m_offscreenPass.depth.view;

                VkFramebufferCreateInfo framebufferCreateInfo = {};
                framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferCreateInfo.renderPass = m_offscreenPass.renderPass;
                framebufferCreateInfo.attachmentCount = 2;
                framebufferCreateInfo.pAttachments = attachments;
                framebufferCreateInfo.width = m_offscreenPass.width;
                framebufferCreateInfo.height = m_offscreenPass.height;
                framebufferCreateInfo.layers = 1;

                VK_CHECK_RESULT(vkCreateFramebuffer(m_device.getLogicalDevice(), &framebufferCreateInfo, nullptr, &m_offscreenPass.framebuffer));

                m_offscreenPass.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                m_offscreenPass.descriptor.imageView = m_offscreenPass.color.view;
                m_offscreenPass.descriptor.sampler = m_offscreenPass.sampler;
            }
    };
}

VULKAN_EXAMPLE_MAIN()
