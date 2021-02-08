#define TINYGLTF_IMPLEMENTATION

#include "VulkanBase.hpp"

#include "VulkanglTFModel.hpp"

namespace VulkanLearning {

    class VulkanExample : public VulkanBase {

        private:

            VulkanglTFModel m_model;

            struct UBOVS {
                glm::mat4 projection;
                glm::mat4 model;
                glm::mat4 view;
            } m_uboVS;

            struct UBOFS {
                glm::vec2 brightnessContrast = glm::vec2(0.5f, 1.8f);
                glm::vec2 range = glm::vec2(0.6f, 1.0f);
                int32_t attachmentIndex = 1;
            } m_uboFS;

            struct {
                VulkanBuffer uboVS;
                VulkanBuffer uboFS;
            } m_ubos;

            VkPipeline m_pipelineWrite;
            VkPipeline m_pipelineRead;

            VkPipelineLayout m_pipelineLayoutWrite;
            VkPipelineLayout m_pipelineLayoutRead;

            VkDescriptorSet m_descriptorSetWrite;
            std::vector<VkDescriptorSet> m_descriptorSetRead;

            VulkanDescriptorSetLayout m_descriptorSetLayoutWrite;
            VulkanDescriptorSetLayout m_descriptorSetLayoutRead;


            struct FramebufferAttachment {
                VkImage image;
                VkDeviceMemory memory;
                VkImageView view;
                VkFormat format;
            };

            struct Attachments {
                FramebufferAttachment color, depth;
            };

            std::vector<Attachments> m_attachments;

        public:
            VulkanExample() {}

            ~VulkanExample() {
                cleanupSwapChain();

                m_ubos.uboVS.cleanup();
                m_ubos.uboFS.cleanup();

                m_descriptorSetLayout.cleanup();

                m_model.~VulkanglTFModel();
            }

            void run() {
                VulkanBase::run();
            }

        private:

            void initVulkan() override {
                m_msaaSamples = 1;
                VulkanBase::initVulkan();

                m_window.setTitle("Input Attachments");
                m_camera.setPosition(glm::vec3(0.0f, 2.5f, 15.0f));
                m_ui.setSubpass(1);

                loadAssets();

                createDescriptorSetLayout();
                createGraphicsPipeline();

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

            void createRenderPass() override {
                m_renderPass = VulkanRenderPass(m_swapChain, m_device);
                m_attachments.resize(m_swapChain.getImages().size());

                for (auto i = 0; i < m_attachments.size(); i++) {
                    createAttachment(
                            VK_FORMAT_R8G8B8A8_UNORM, 
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
                            &m_attachments[i].color);

                    createAttachment(
                            m_device.findDepthFormat(), 
                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                            &m_attachments[i].depth);
                }


                VkAttachmentDescription colorAttachmentResolve{};
                colorAttachmentResolve.format = m_swapChain.getImageFormat();
                colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
                colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                VkAttachmentDescription colorAttachment{};
                colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
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

                std::vector<VkAttachmentDescription> attachments = {
                    colorAttachmentResolve,
                    colorAttachment,
                    depthAttachment
                };

                VkAttachmentReference colorAttachmentResolveRef{};
                colorAttachmentResolveRef.attachment = 0;
                colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentReference colorAttachmentRef{};
                colorAttachmentRef.attachment = 1;
                colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentReference depthAttachmentRef{};
                depthAttachmentRef.attachment = 2;
                depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                std::vector<VkSubpassDescription> subpassDescriptions(2);
                subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpassDescriptions[0].colorAttachmentCount = 1;
                subpassDescriptions[0].pColorAttachments = &colorAttachmentRef;
                subpassDescriptions[0].pDepthStencilAttachment = &depthAttachmentRef;

                subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpassDescriptions[1].colorAttachmentCount = 1;
                subpassDescriptions[1].pColorAttachments = &colorAttachmentResolveRef;

                // Pass color and depth attachments to using input attachments in fragment shader
                VkAttachmentReference inputReferences[2];
                inputReferences[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                inputReferences[1] = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                subpassDescriptions[1].inputAttachmentCount = 2;
                subpassDescriptions[1].pInputAttachments = inputReferences;

                std::vector<VkSubpassDependency> dependencies(3);
                dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[0].dstSubpass = 0;
                dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                // This dependency transitions the input attachment from color attachment to shader read
                dependencies[1].srcSubpass = 0;
                dependencies[1].dstSubpass = 1;
                dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                dependencies[2].srcSubpass = 0;
                dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                m_renderPass.create(dependencies, attachments, subpassDescriptions);
            }

            void createGraphicsPipeline() override {
                VkPipelineLayoutCreateInfo pipelineLayoutWriteInfo{};
                pipelineLayoutWriteInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutWriteInfo.setLayoutCount = 1;
                pipelineLayoutWriteInfo.pSetLayouts = m_descriptorSetLayoutWrite.getDescriptorSetLayoutPointer();
                vkCreatePipelineLayout(
                        m_device.getLogicalDevice(), 
                        &pipelineLayoutWriteInfo, 
                        nullptr, 
                        &m_pipelineLayoutWrite);

                VkPipelineLayoutCreateInfo pipelineLayoutReadInfo{};
                pipelineLayoutReadInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutReadInfo.setLayoutCount = 1;
                pipelineLayoutReadInfo.pSetLayouts = m_descriptorSetLayoutRead.getDescriptorSetLayoutPointer();
                vkCreatePipelineLayout(
                        m_device.getLogicalDevice(), 
                        &pipelineLayoutReadInfo, 
                        nullptr, 
                        &m_pipelineLayoutRead);

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

                VulkanShaderModule vertShaderModuleWrite = 
                    VulkanShaderModule(
                            "src/shaders/inputAttachments/inputAttachmentsWriteVert.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_VERTEX_BIT);
                VulkanShaderModule fragShaderModuleWrite = 
                    VulkanShaderModule(
                            "src/shaders/inputAttachments/inputAttachmentsWriteFrag.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_FRAGMENT_BIT);

                VulkanShaderModule vertShaderModuleRead = 
                    VulkanShaderModule(
                            "src/shaders/inputAttachments/inputAttachmentsReadVert.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_VERTEX_BIT);
                VulkanShaderModule fragShaderModuleRead = 
                    VulkanShaderModule(
                            "src/shaders/inputAttachments/inputAttachmentsReadFrag.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_FRAGMENT_BIT);

                VkPipelineShaderStageCreateInfo shaderStagesWrite[] = {
                    vertShaderModuleWrite.getStageCreateInfo(), 
                    fragShaderModuleWrite.getStageCreateInfo()
                };
                VkPipelineShaderStageCreateInfo shaderStagesRead[] = {
                    vertShaderModuleRead.getStageCreateInfo(), 
                    fragShaderModuleRead.getStageCreateInfo()
                };

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

                // 1rst pipeline : Write
                pipelineInfo.subpass = 0;
                pipelineInfo.layout = m_pipelineLayoutWrite;
                pipelineInfo.pStages = shaderStagesWrite;
                pipelineInfo.pVertexInputState = Vertex::getPipelineVertexInputState({
                        VertexComponent::Position,
                        VertexComponent::Color,
                        VertexComponent::Normal});

                VK_CHECK_RESULT(vkCreateGraphicsPipelines(
                            m_device.getLogicalDevice(), 
                            VK_NULL_HANDLE, 
                            1, 
                            &pipelineInfo, 
                            nullptr, 
                            &m_pipelineWrite));

                // 2nd pipeline : Read
                pipelineInfo.subpass = 1;
                pipelineInfo.layout = m_pipelineLayoutRead;
                pipelineInfo.pStages = shaderStagesRead;

                VkPipelineVertexInputStateCreateInfo emptyVertexInputStateCreateInfo = {};
                emptyVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                pipelineInfo.pVertexInputState = &emptyVertexInputStateCreateInfo;
                rasterizer.cullMode = VK_CULL_MODE_NONE;
                depthStencilState.depthWriteEnable = VK_FALSE;

                VK_CHECK_RESULT(vkCreateGraphicsPipelines(
                            m_device.getLogicalDevice(), 
                            VK_NULL_HANDLE, 
                            1, 
                            &pipelineInfo, 
                            nullptr, 
                            &m_pipelineRead));

                vertShaderModuleRead.cleanup(&m_device);
                fragShaderModuleRead.cleanup(&m_device);
                vertShaderModuleWrite.cleanup(&m_device);
                fragShaderModuleWrite.cleanup(&m_device);
            }

            void createUniformBuffers() {
                m_ubos.uboVS = VulkanBuffer(m_device);
                m_ubos.uboFS = VulkanBuffer(m_device);

                m_ubos.uboVS.createBuffer(
                        sizeof(m_uboVS), 
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                m_ubos.uboFS.createBuffer(
                        sizeof(m_uboFS), 
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                m_ubos.uboVS.map();
                m_ubos.uboFS.map();

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

                VkClearValue clearValues[3];
                clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
                clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
                clearValues[2].depthStencil = { 1.0f, 0 };

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

                VkRenderPassBeginInfo renderPassBeginInfo = {};
                renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.renderPass = m_renderPass.getRenderPass();
                renderPassBeginInfo.renderArea.offset.x = 0;
                renderPassBeginInfo.renderArea.offset.y = 0;
                renderPassBeginInfo.renderArea.extent.width = m_swapChain.getExtent().width;
                renderPassBeginInfo.renderArea.extent.height = m_swapChain.getExtent().height;
                renderPassBeginInfo.clearValueCount = 3;
                renderPassBeginInfo.pClearValues = clearValues;

                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = 0;
                beginInfo.pInheritanceInfo = nullptr;

                for (int32_t i = 0; i < m_commandBuffers.size(); ++i)
                {
                    renderPassBeginInfo.framebuffer = m_framebuffers[i];

                    VK_CHECK_RESULT(vkBeginCommandBuffer(m_commandBuffers[i].getCommandBuffer(), &beginInfo));

                    vkCmdBeginRenderPass(m_commandBuffers[i].getCommandBuffer(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdSetViewport(m_commandBuffers[i].getCommandBuffer(), 0, 1, &viewport);
                    vkCmdSetScissor(m_commandBuffers[i].getCommandBuffer(), 0, 1, &scissor);

                    // 1rst subpass
                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelineWrite);

                    vkCmdBindDescriptorSets(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelineLayoutWrite,
                            0, 
                            1, 
                            &m_descriptorSetWrite,
                            0, 
                            nullptr);

                    m_model.draw(m_commandBuffers[i].getCommandBuffer());

                    // 2nd subpass
                    vkCmdNextSubpass(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelineRead);

                    vkCmdBindDescriptorSets(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelineLayoutRead,
                            0, 
                            1, 
                            &m_descriptorSetRead[i],
                            0, 
                            nullptr);

                    // Render a full screen quad using input attachments written in previous subpass
                    vkCmdDraw(m_commandBuffers[i].getCommandBuffer(), 3, 1, 0, 0);

                    drawUI(m_commandBuffers[i].getCommandBuffer());

                    vkCmdEndRenderPass(m_commandBuffers[i].getCommandBuffer());
                    VK_CHECK_RESULT(vkEndCommandBuffer(m_commandBuffers[i].getCommandBuffer()));
                }
            }

            void createDescriptorSetLayout() override {
                m_descriptorSetLayoutWrite = VulkanDescriptorSetLayout(m_device);
                m_descriptorSetLayoutRead = VulkanDescriptorSetLayout(m_device);

                std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindingsWrite(1);
                descriptorSetLayoutBindingsWrite[0].binding = 0;
                descriptorSetLayoutBindingsWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorSetLayoutBindingsWrite[0].descriptorCount = 1;
                descriptorSetLayoutBindingsWrite[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                descriptorSetLayoutBindingsWrite[0].pImmutableSamplers = nullptr;

                std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindingsRead(3);
                descriptorSetLayoutBindingsRead[0].binding = 0;
                descriptorSetLayoutBindingsRead[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                descriptorSetLayoutBindingsRead[0].descriptorCount = 1;
                descriptorSetLayoutBindingsRead[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                descriptorSetLayoutBindingsRead[0].pImmutableSamplers = nullptr;
                descriptorSetLayoutBindingsRead[1].binding = 1;
                descriptorSetLayoutBindingsRead[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                descriptorSetLayoutBindingsRead[1].descriptorCount = 1;
                descriptorSetLayoutBindingsRead[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                descriptorSetLayoutBindingsRead[1].pImmutableSamplers = nullptr;
                descriptorSetLayoutBindingsRead[2].binding = 2;
                descriptorSetLayoutBindingsRead[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorSetLayoutBindingsRead[2].descriptorCount = 1;
                descriptorSetLayoutBindingsRead[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                descriptorSetLayoutBindingsRead[2].pImmutableSamplers = nullptr;

                m_descriptorSetLayoutWrite.create(descriptorSetLayoutBindingsWrite);
                m_descriptorSetLayoutRead.create(descriptorSetLayoutBindingsRead);
            }

            void createDescriptorPool() override {
                m_descriptorPool = VulkanDescriptorPool(m_device, m_swapChain);

                std::vector<VkDescriptorPoolSize> poolSizes = std::vector<VkDescriptorPoolSize>(3);

                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = m_attachments.size() + 1;
                poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[1].descriptorCount = m_attachments.size() + 1;
                poolSizes[2].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                poolSizes[2].descriptorCount = m_attachments.size() * 2 + 1;

                m_descriptorPool.create(poolSizes, m_attachments.size() + 1);
            }

            void createDescriptorSets() override {
                VkDescriptorSetAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool = m_descriptorPool.getDescriptorPool();
                allocInfo.descriptorSetCount = 1;
                allocInfo.pSetLayouts = m_descriptorSetLayoutWrite.getDescriptorSetLayoutPointer();

                VK_CHECK_RESULT(vkAllocateDescriptorSets(
                            m_device.getLogicalDevice(), 
                            &allocInfo, 
                            &m_descriptorSetWrite));

                std::vector<VkWriteDescriptorSet> descriptorWrites(1);

                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[0].descriptorCount = 1;
                descriptorWrites[0].pBufferInfo = m_ubos.uboVS.getDescriptorPointer();
                descriptorWrites[0].dstSet = m_descriptorSetWrite;

                vkUpdateDescriptorSets(
                        m_device.getLogicalDevice(), 
                        static_cast<uint32_t>(descriptorWrites.size()), 
                        descriptorWrites.data(), 
                        0, 
                        NULL);

                m_descriptorSetRead.resize(m_attachments.size());
                for (auto i = 0; i < m_descriptorSetRead.size(); i++) {
                    VkDescriptorSetAllocateInfo allocInfo{};
                    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    allocInfo.descriptorPool = m_descriptorPool.getDescriptorPool();
                    allocInfo.descriptorSetCount = 1;
                    allocInfo.pSetLayouts = m_descriptorSetLayoutRead.getDescriptorSetLayoutPointer();

                    VK_CHECK_RESULT(vkAllocateDescriptorSets(
                                m_device.getLogicalDevice(), 
                                &allocInfo, 
                                &m_descriptorSetRead[i]));

                    // Input attachments 
                    std::vector<VkDescriptorImageInfo> descriptors(2);
                    descriptors[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    descriptors[0].imageView = m_attachments[i].color.view;
                    descriptors[0].sampler = VK_NULL_HANDLE;
                    descriptors[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    descriptors[1].imageView = m_attachments[i].depth.view;
                    descriptors[1].sampler = VK_NULL_HANDLE;

                    std::vector<VkWriteDescriptorSet> descriptorWrites(3);

                    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrites[0].dstBinding = 0;
                    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                    descriptorWrites[0].descriptorCount = 1;
                    descriptorWrites[0].pImageInfo = &descriptors[0];
                    descriptorWrites[0].dstSet = m_descriptorSetRead[i];

                    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrites[1].dstBinding = 1;
                    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                    descriptorWrites[1].descriptorCount = 1;
                    descriptorWrites[1].pImageInfo = &descriptors[1];
                    descriptorWrites[1].dstSet = m_descriptorSetRead[i];

                    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrites[2].dstBinding = 2;
                    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    descriptorWrites[2].descriptorCount = 1;
                    descriptorWrites[2].pBufferInfo = m_ubos.uboFS.getDescriptorPointer();
                    descriptorWrites[2].dstSet = m_descriptorSetRead[i];

                    vkUpdateDescriptorSets(
                            m_device.getLogicalDevice(), 
                            static_cast<uint32_t>(descriptorWrites.size()), 
                            descriptorWrites.data(), 
                            0, 
                            NULL);
                }
            }

            void updateUniformBuffers() {
                m_uboVS.projection = glm::perspective(
                        glm::radians(m_camera.getZoom()), 
                        m_swapChain.getExtent().width / (float) m_swapChain.getExtent().height, 
                        0.1f,  100.0f);
                m_uboVS.projection[1][1] *= -1;
                m_uboVS.view = m_camera.getViewMatrix();
                m_uboVS.model = glm::mat4(1.0f);

                memcpy(m_ubos.uboVS.getMappedMemory(), &m_uboVS, sizeof(m_uboVS));
                memcpy(m_ubos.uboFS.getMappedMemory(), &m_uboFS, sizeof(m_uboFS));
            }

            void loadAssets() {
                uint32_t glTFLoadingFlags = FileLoadingFlags::PreTransformVertices | FileLoadingFlags::PreMultiplyVertexColors;
                m_model.loadFromFile("src/models/treasure_smooth.gltf", &m_device, m_device.getGraphicsQueue(), glTFLoadingFlags);
            }

            void OnUpdateUI (UI *ui) override {
                if (ui->header("Settings")) {
                    ui->text("Input attachment");
                    if (ui->comboBox("##attachment", &m_uboFS.attachmentIndex, { "color", "depth" })) {
                        updateUniformBuffers();
                    }
                    switch (m_uboFS.attachmentIndex) {
                        case 0:
                            ui->text("Brightness");
                            if (ui->sliderFloat("##b", &m_uboFS.brightnessContrast[0], 0.0f, 2.0f)) {
                                updateUniformBuffers();
                            }
                            ui->text("Contrast");
                            if (ui->sliderFloat("##c", &m_uboFS.brightnessContrast[1], 0.0f, 4.0f)) {
                                updateUniformBuffers();
                            }
                            break;
                        case 1:
                            ui->text("Visible range");
                            if (ui->sliderFloat("min", &m_uboFS.range[0], 0.0f, m_uboFS.range[1])) {
                                updateUniformBuffers();
                            }
                            if (ui->sliderFloat("max", &m_uboFS.range[1], m_uboFS.range[0], 1.0f)) {
                                updateUniformBuffers();
                            }
                            break;
                    }
                }
            }

            void createFramebuffers() override {
                VkImageView views[3];

                VkFramebufferCreateInfo framebufferCreateInfo = {};
                framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferCreateInfo.renderPass = m_renderPass.getRenderPass();
                framebufferCreateInfo.attachmentCount = 3;
                framebufferCreateInfo.pAttachments = views;
                framebufferCreateInfo.width = m_swapChain.getExtent().width;
                framebufferCreateInfo.height = m_swapChain.getExtent().height;
                framebufferCreateInfo.layers = 1;

                m_framebuffers.resize(m_swapChain.getImages().size());

                for (uint32_t i = 0; i < m_framebuffers.size(); i++) {
                    views[0] = m_swapChain.getImagesViews()[i];
                    views[1] = m_attachments[i].color.view;
                    views[2] = m_attachments[i].depth.view;
                    vkCreateFramebuffer(m_device.getLogicalDevice(), &framebufferCreateInfo, nullptr, &m_framebuffers[i]);
                }
            }

            void createAttachment(VkFormat format, VkImageUsageFlags usage, FramebufferAttachment *attachment) {
                VkImageAspectFlags aspectMask = 0;
                VkImageLayout imageLayout;

                attachment->format = format;

                if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
                    aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                    aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
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
                imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
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
    };
}

VULKAN_EXAMPLE_MAIN()
