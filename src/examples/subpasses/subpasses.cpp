#define TINYGLTF_IMPLEMENTATION

#include "VulkanBase.hpp"

#include "VulkanglTFModel.hpp"


namespace VulkanLearning {

#define NUM_LIGHTS 64

    class VulkanExample : public VulkanBase {

        private:

            VulkanTexture2D m_glassTexture;

            struct {
                VulkanglTFModel scene;
                VulkanglTFModel transparent;
            } m_models;

            struct UBOMatrices {
                glm::mat4 projection;
                glm::mat4 model;
                glm::mat4 view;
            } m_uboMatrices;

            struct Light { 
                glm::vec4 position;
                glm::vec3 color;
                float radius;
            };

            struct {
                glm::vec4 viewPos;
                Light lights[NUM_LIGHTS];
            } m_uboLights;

            struct {
                VulkanBuffer uboMatrices;
                VulkanBuffer uboLights;
            } m_ubos;

            struct {
                VkPipeline offscreen;
                VkPipeline composition;
                VkPipeline transparent;
            } m_pipelines;
            
            struct {
                VkPipelineLayout offscreen;
                VkPipelineLayout composition;
                VkPipelineLayout transparent;
            } m_pipelineLayouts;

            struct {
                VkDescriptorSet scene;
                VkDescriptorSet composition;
                VkDescriptorSet transparent;
            } m_descriptorSets;

            struct {
                VulkanDescriptorSetLayout scene;
                VulkanDescriptorSetLayout composition;
                VulkanDescriptorSetLayout transparent;
            } m_descriptorSetLayouts;

            struct FramebufferAttachment {
                VkImage image;
                VkDeviceMemory memory;
                VkImageView view;
                VkFormat format;
            };

            struct Attachments {
                FramebufferAttachment position, normal, albedo;
                int32_t width;
                int32_t height;
            } m_attachments;

        public:
            VulkanExample() {}

            ~VulkanExample() {
                cleanupSwapChain();

                m_ubos.uboMatrices.cleanup();
                m_ubos.uboLights.cleanup();

                m_descriptorSetLayout.cleanup();

                m_models.scene.~VulkanglTFModel();
                m_models.transparent.~VulkanglTFModel();
            }

            void run() {
                VulkanBase::run();
            }

        private:

            void initVulkan() override {
                m_msaaSamples = 1;
                VulkanBase::initVulkan();

                m_window.setTitle("Subpasses");
                m_camera.setPosition(glm::vec3(-3.40f, 0.74f, 6.67f));
                m_ui.setSubpass(2);

                loadAssets();

                createDescriptorSetLayout();
                createGraphicsPipeline();

                createUniformBuffers();

                initLights();
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

                m_attachments.width = m_swapChain.getExtent().width;
                m_attachments.height = m_swapChain.getExtent().height;
                createGAttachments();

                VkAttachmentDescription colorAttachmentResolve{};
                colorAttachmentResolve.format = m_swapChain.getImageFormat();
                colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
                colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                // Deferred attachments
                VkAttachmentDescription positionAttachment{};
                positionAttachment.format = m_attachments.position.format;
                positionAttachment.samples = m_device.getMsaaSamples();
                positionAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                positionAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                positionAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                positionAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                positionAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                positionAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentDescription normalAttachment{};
                normalAttachment.format = m_attachments.normal.format;
                normalAttachment.samples = m_device.getMsaaSamples();
                normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                normalAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                normalAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                normalAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                normalAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentDescription albedoAttachment{};
                albedoAttachment.format = m_attachments.albedo.format;
                albedoAttachment.samples = m_device.getMsaaSamples();
                albedoAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                albedoAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                albedoAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                albedoAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                albedoAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                albedoAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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
                    positionAttachment,
                    normalAttachment,
                    albedoAttachment,
                    depthAttachment
                };

                std::vector<VkSubpassDescription> subpassDescriptions(3);
                // 1rst: filling G-Buffers
                VkAttachmentReference colorReferences[4];
                colorReferences[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
                colorReferences[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
                colorReferences[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
                colorReferences[3] = { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

                VkAttachmentReference depthAttachmentRef = { 4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
                subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpassDescriptions[0].colorAttachmentCount = 4;
                subpassDescriptions[0].pColorAttachments = colorReferences;
                subpassDescriptions[0].pDepthStencilAttachment = &depthAttachmentRef;

                // 2nd: Use G-Buffers to get the composition
                VkAttachmentReference colorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

                VkAttachmentReference inputReferences[3];
                inputReferences[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                inputReferences[1] = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                inputReferences[2] = { 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

                subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpassDescriptions[1].colorAttachmentCount = 1;
                subpassDescriptions[1].pColorAttachments = &colorRef;
                subpassDescriptions[1].inputAttachmentCount = 3;
                subpassDescriptions[1].pInputAttachments = inputReferences;

                // 3rd: Transparency
                subpassDescriptions[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpassDescriptions[2].colorAttachmentCount = 1;
                subpassDescriptions[2].pColorAttachments = &colorRef;
                subpassDescriptions[2].pDepthStencilAttachment = &depthAttachmentRef;
                subpassDescriptions[2].inputAttachmentCount = 1;
                subpassDescriptions[2].pInputAttachments = inputReferences;

                std::vector<VkSubpassDependency> dependencies(4);
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

                dependencies[2].srcSubpass = 1;
                dependencies[2].dstSubpass = 2;
                dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                dependencies[3].srcSubpass = 0;
                dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[3].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[3].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                m_renderPass.create(dependencies, attachments, subpassDescriptions);
            }

            void createGraphicsPipeline() override {
                VkPipelineLayoutCreateInfo pipelineLayoutOffscreenCI{};
                pipelineLayoutOffscreenCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutOffscreenCI.setLayoutCount = 1;
                pipelineLayoutOffscreenCI.pSetLayouts = m_descriptorSetLayouts.scene.getDescriptorSetLayoutPointer();

                vkCreatePipelineLayout(
                        m_device.getLogicalDevice(), 
                        &pipelineLayoutOffscreenCI, 
                        nullptr, 
                        &m_pipelineLayouts.offscreen);

                VkPipelineLayoutCreateInfo pipelineLayoutCompositionCI{};
                pipelineLayoutCompositionCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutCompositionCI.setLayoutCount = 1;
                pipelineLayoutCompositionCI.pSetLayouts = m_descriptorSetLayouts.composition.getDescriptorSetLayoutPointer();

                vkCreatePipelineLayout(
                        m_device.getLogicalDevice(), 
                        &pipelineLayoutCompositionCI, 
                        nullptr, 
                        &m_pipelineLayouts.composition);

                VkPipelineLayoutCreateInfo pipelineLayoutTransparencyCI{};
                pipelineLayoutTransparencyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutTransparencyCI.setLayoutCount = 1;
                pipelineLayoutTransparencyCI.pSetLayouts = m_descriptorSetLayouts.transparent.getDescriptorSetLayoutPointer();

                vkCreatePipelineLayout(
                        m_device.getLogicalDevice(), 
                        &pipelineLayoutTransparencyCI, 
                        nullptr, 
                        &m_pipelineLayouts.transparent);

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

                std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(4);
                colorBlendAttachments[0].colorWriteMask = 0xf;
                colorBlendAttachments[0].blendEnable = VK_FALSE;
                colorBlendAttachments[1].colorWriteMask = 0xf;
                colorBlendAttachments[1].blendEnable = VK_FALSE;
                colorBlendAttachments[2].colorWriteMask = 0xf;
                colorBlendAttachments[2].blendEnable = VK_FALSE;
                colorBlendAttachments[3].colorWriteMask = 0xf;
                colorBlendAttachments[3].blendEnable = VK_FALSE;

                VkPipelineColorBlendStateCreateInfo colorBlending{};
                colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
                colorBlending.pAttachments = colorBlendAttachments.data();

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

                // Final pipeline
                VulkanShaderModule vertShaderModuleGBuffer = 
                    VulkanShaderModule(
                            "src/shaders/subpasses/gbufferVert.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_VERTEX_BIT);
                VulkanShaderModule fragShaderModuleGBuffer = 
                    VulkanShaderModule(
                            "src/shaders/subpasses/gbufferFrag.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_FRAGMENT_BIT);

                VkPipelineShaderStageCreateInfo shaderStagesGBuffer[] = {
                    vertShaderModuleGBuffer.getStageCreateInfo(), 
                    fragShaderModuleGBuffer.getStageCreateInfo()
                };

                pipelineInfo.subpass = 0;
                pipelineInfo.layout = m_pipelineLayouts.offscreen;
                pipelineInfo.pStages = shaderStagesGBuffer;
                pipelineInfo.pVertexInputState = Vertex::getPipelineVertexInputState({
                        VertexComponent::Position,
                        VertexComponent::Color,
                        VertexComponent::Normal,
                        VertexComponent::UV});

                VK_CHECK_RESULT(vkCreateGraphicsPipelines(
                            m_device.getLogicalDevice(), 
                            VK_NULL_HANDLE, 
                            1, 
                            &pipelineInfo, 
                            nullptr, 
                            &m_pipelines.offscreen));

                // Composition pipeline
                VulkanShaderModule vertShaderModuleComposition = 
                    VulkanShaderModule(
                            "src/shaders/subpasses/compositionVert.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_VERTEX_BIT);
                VulkanShaderModule fragShaderModuleComposition = 
                    VulkanShaderModule(
                            "src/shaders/subpasses/compositionFrag.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_FRAGMENT_BIT);

                VkPipelineShaderStageCreateInfo shaderStagesComposition[] = {
                    vertShaderModuleComposition.getStageCreateInfo(), 
                    fragShaderModuleComposition.getStageCreateInfo()
                };
                pipelineInfo.subpass = 1;
                pipelineInfo.layout = m_pipelineLayouts.composition;
                pipelineInfo.pStages = shaderStagesComposition;

                VkPipelineVertexInputStateCreateInfo emptyVertexInputStateCreateInfo = {};
                emptyVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                pipelineInfo.pVertexInputState = &emptyVertexInputStateCreateInfo;

                rasterizer.cullMode = VK_CULL_MODE_NONE;
                depthStencilState.depthWriteEnable = VK_FALSE;

                VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
                colorBlendAttachment.colorWriteMask = 0xf;
                colorBlendAttachment.blendEnable = VK_FALSE;

                colorBlending.attachmentCount = 1;
                colorBlending.pAttachments = &colorBlendAttachment;

                pipelineInfo.pColorBlendState = &colorBlending;

                // Pass number of lights through specialization entry
                VkSpecializationMapEntry specializationEntry = {};
                specializationEntry.constantID = 0;
                specializationEntry.offset = 0;
                specializationEntry.size = sizeof(uint32_t);

                uint32_t lightNumber = NUM_LIGHTS;
                VkSpecializationInfo specializationInfo = {};
                specializationInfo.dataSize = sizeof(lightNumber);
                specializationInfo.mapEntryCount = 1;
                specializationInfo.pMapEntries = &specializationEntry;
                specializationInfo.pData = &lightNumber;

                shaderStagesComposition->pSpecializationInfo = &specializationInfo;

                VK_CHECK_RESULT(vkCreateGraphicsPipelines(
                            m_device.getLogicalDevice(), 
                            VK_NULL_HANDLE, 
                            1, 
                            &pipelineInfo, 
                            nullptr, 
                            &m_pipelines.composition));

                // Transparent (forward) pipeline
                VulkanShaderModule vertShaderModuleTransparent = 
                    VulkanShaderModule(
                            "src/shaders/subpasses/transparentVert.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_VERTEX_BIT);
                VulkanShaderModule fragShaderModuleTransparent = 
                    VulkanShaderModule(
                            "src/shaders/subpasses/transparentFrag.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_FRAGMENT_BIT);

                VkPipelineShaderStageCreateInfo shaderStagesTransparent[] = {
                    vertShaderModuleTransparent.getStageCreateInfo(), 
                    fragShaderModuleTransparent.getStageCreateInfo()
                };
                pipelineInfo.subpass = 2;
                pipelineInfo.layout = m_pipelineLayouts.transparent;
                pipelineInfo.pStages = shaderStagesTransparent;

                colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                colorBlendAttachment.blendEnable = VK_TRUE;
                colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
                colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

                pipelineInfo.pVertexInputState = Vertex::getPipelineVertexInputState({
                        VertexComponent::Position,
                        VertexComponent::Color,
                        VertexComponent::Normal,
                        VertexComponent::UV});

                VK_CHECK_RESULT(vkCreateGraphicsPipelines(
                            m_device.getLogicalDevice(), 
                            VK_NULL_HANDLE, 
                            1, 
                            &pipelineInfo, 
                            nullptr, 
                            &m_pipelines.transparent));


                vertShaderModuleGBuffer.cleanup(&m_device);
                fragShaderModuleGBuffer.cleanup(&m_device);
                vertShaderModuleComposition.cleanup(&m_device);
                fragShaderModuleComposition.cleanup(&m_device);
                vertShaderModuleTransparent.cleanup(&m_device);
                fragShaderModuleTransparent.cleanup(&m_device);
            }

            void createUniformBuffers() {
                m_ubos.uboMatrices = VulkanBuffer(m_device);
                m_ubos.uboLights = VulkanBuffer(m_device);

                m_ubos.uboMatrices.createBuffer(
                        sizeof(m_uboMatrices), 
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                m_ubos.uboLights.createBuffer(
                        sizeof(m_uboLights), 
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                m_ubos.uboMatrices.map();
                m_ubos.uboLights.map();

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

                VkClearValue clearValues[5];
                clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
                clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
                clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
                clearValues[3].color = { 0.0f, 0.0f, 0.0f, 1.0f };
                clearValues[4].depthStencil = { 1.0f, 0 };

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
                renderPassBeginInfo.clearValueCount = 5;
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
                            m_pipelines.offscreen);

                    vkCmdBindDescriptorSets(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelineLayouts.offscreen,
                            0, 
                            1, 
                            &m_descriptorSets.scene,
                            0, 
                            nullptr);

                    m_models.scene.draw(m_commandBuffers[i].getCommandBuffer());

                    // 2nd subpass
                    vkCmdNextSubpass(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelines.composition);

                    vkCmdBindDescriptorSets(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelineLayouts.composition,
                            0, 
                            1, 
                            &m_descriptorSets.composition,
                            0, 
                            nullptr);

                    vkCmdDraw(m_commandBuffers[i].getCommandBuffer(), 3, 1, 0, 0);

                    // 3rd subpass
                    vkCmdNextSubpass(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelines.transparent);

                    vkCmdBindDescriptorSets(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelineLayouts.transparent,
                            0, 
                            1, 
                            &m_descriptorSets.transparent,
                            0, 
                            nullptr);

                    m_models.transparent.draw(m_commandBuffers[i].getCommandBuffer());

                    drawUI(m_commandBuffers[i].getCommandBuffer());

                    vkCmdEndRenderPass(m_commandBuffers[i].getCommandBuffer());
                    VK_CHECK_RESULT(vkEndCommandBuffer(m_commandBuffers[i].getCommandBuffer()));
                }
            }

            void createDescriptorSetLayout() override {
                m_descriptorSetLayouts.scene = VulkanDescriptorSetLayout(m_device);
                m_descriptorSetLayouts.composition = VulkanDescriptorSetLayout(m_device);
                m_descriptorSetLayouts.transparent = VulkanDescriptorSetLayout(m_device);

                std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindingsScene(1);
                descriptorSetLayoutBindingsScene[0].binding = 0;
                descriptorSetLayoutBindingsScene[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorSetLayoutBindingsScene[0].descriptorCount = 1;
                descriptorSetLayoutBindingsScene[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                descriptorSetLayoutBindingsScene[0].pImmutableSamplers = nullptr;

                std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindingsComposition(4);
                descriptorSetLayoutBindingsComposition[0].binding = 0;
                descriptorSetLayoutBindingsComposition[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                descriptorSetLayoutBindingsComposition[0].descriptorCount = 1;
                descriptorSetLayoutBindingsComposition[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                descriptorSetLayoutBindingsComposition[0].pImmutableSamplers = nullptr;
                descriptorSetLayoutBindingsComposition[1].binding = 1;
                descriptorSetLayoutBindingsComposition[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                descriptorSetLayoutBindingsComposition[1].descriptorCount = 1;
                descriptorSetLayoutBindingsComposition[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                descriptorSetLayoutBindingsComposition[1].pImmutableSamplers = nullptr;
                descriptorSetLayoutBindingsComposition[2].binding = 2;
                descriptorSetLayoutBindingsComposition[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                descriptorSetLayoutBindingsComposition[2].descriptorCount = 1;
                descriptorSetLayoutBindingsComposition[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                descriptorSetLayoutBindingsComposition[2].pImmutableSamplers = nullptr;
                descriptorSetLayoutBindingsComposition[3].binding = 3;
                descriptorSetLayoutBindingsComposition[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorSetLayoutBindingsComposition[3].descriptorCount = 1;
                descriptorSetLayoutBindingsComposition[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                descriptorSetLayoutBindingsComposition[3].pImmutableSamplers = nullptr;

                std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindingsTransparent(3);
                descriptorSetLayoutBindingsTransparent[0].binding = 0;
                descriptorSetLayoutBindingsTransparent[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorSetLayoutBindingsTransparent[0].descriptorCount = 1;
                descriptorSetLayoutBindingsTransparent[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                descriptorSetLayoutBindingsTransparent[0].pImmutableSamplers = nullptr;
                descriptorSetLayoutBindingsTransparent[1].binding = 1;
                descriptorSetLayoutBindingsTransparent[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                descriptorSetLayoutBindingsTransparent[1].descriptorCount = 1;
                descriptorSetLayoutBindingsTransparent[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                descriptorSetLayoutBindingsTransparent[1].pImmutableSamplers = nullptr;
                descriptorSetLayoutBindingsTransparent[2].binding = 2;
                descriptorSetLayoutBindingsTransparent[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorSetLayoutBindingsTransparent[2].descriptorCount = 1;
                descriptorSetLayoutBindingsTransparent[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                descriptorSetLayoutBindingsTransparent[2].pImmutableSamplers = nullptr;

                m_descriptorSetLayouts.scene.create(descriptorSetLayoutBindingsScene);
                m_descriptorSetLayouts.composition.create(descriptorSetLayoutBindingsComposition);
                m_descriptorSetLayouts.transparent.create(descriptorSetLayoutBindingsTransparent);
            }

            void createDescriptorPool() override {
                m_descriptorPool = VulkanDescriptorPool(m_device, m_swapChain);

                std::vector<VkDescriptorPoolSize> poolSizes = std::vector<VkDescriptorPoolSize>(3);

                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = 4;
                poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[1].descriptorCount = 4;
                poolSizes[2].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                poolSizes[2].descriptorCount = 4;

                m_descriptorPool.create(poolSizes, 4);
            }

            void createDescriptorSets() override {
                // Scene
                VkDescriptorSetAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool = m_descriptorPool.getDescriptorPool();
                allocInfo.descriptorSetCount = 1;
                allocInfo.pSetLayouts = m_descriptorSetLayouts.scene.getDescriptorSetLayoutPointer();

                VK_CHECK_RESULT(vkAllocateDescriptorSets(
                            m_device.getLogicalDevice(), 
                            &allocInfo, 
                            &m_descriptorSets.scene));

                std::vector<VkWriteDescriptorSet> descriptorScene(1);
                descriptorScene[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorScene[0].dstBinding = 0;
                descriptorScene[0].dstArrayElement = 0;
                descriptorScene[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorScene[0].descriptorCount = 1;
                descriptorScene[0].pBufferInfo = m_ubos.uboMatrices.getDescriptorPointer();
                descriptorScene[0].dstSet = m_descriptorSets.scene;

                vkUpdateDescriptorSets(
                        m_device.getLogicalDevice(), 
                        static_cast<uint32_t>(descriptorScene.size()), 
                        descriptorScene.data(), 
                        0, 
                        NULL);

                // Composition
                allocInfo.pSetLayouts = m_descriptorSetLayouts.composition.getDescriptorSetLayoutPointer();

                VK_CHECK_RESULT(vkAllocateDescriptorSets(
                            m_device.getLogicalDevice(), 
                            &allocInfo, 
                            &m_descriptorSets.composition));

                std::vector<VkDescriptorImageInfo> texDescriptorImageInfo(3); 
                texDescriptorImageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                texDescriptorImageInfo[0].imageView = m_attachments.position.view;
                texDescriptorImageInfo[0].sampler = VK_NULL_HANDLE;
                texDescriptorImageInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                texDescriptorImageInfo[1].imageView = m_attachments.normal.view;
                texDescriptorImageInfo[1].sampler = VK_NULL_HANDLE;
                texDescriptorImageInfo[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                texDescriptorImageInfo[2].imageView = m_attachments.albedo.view;
                texDescriptorImageInfo[2].sampler = VK_NULL_HANDLE;

                std::vector<VkWriteDescriptorSet> descriptorComposition(4);
                descriptorComposition[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorComposition[0].dstBinding = 0;
                descriptorComposition[0].dstArrayElement = 0;
                descriptorComposition[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                descriptorComposition[0].descriptorCount = 1;
                descriptorComposition[0].pImageInfo = &texDescriptorImageInfo[0];
                descriptorComposition[0].dstSet = m_descriptorSets.composition;

                descriptorComposition[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorComposition[1].dstBinding = 1;
                descriptorComposition[1].dstArrayElement = 0;
                descriptorComposition[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                descriptorComposition[1].descriptorCount = 1;
                descriptorComposition[1].pImageInfo = &texDescriptorImageInfo[1];
                descriptorComposition[1].dstSet = m_descriptorSets.composition;

                descriptorComposition[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorComposition[2].dstBinding = 2;
                descriptorComposition[2].dstArrayElement = 0;
                descriptorComposition[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                descriptorComposition[2].descriptorCount = 1;
                descriptorComposition[2].pImageInfo = &texDescriptorImageInfo[2];
                descriptorComposition[2].dstSet = m_descriptorSets.composition;

                descriptorComposition[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorComposition[3].dstBinding = 3;
                descriptorComposition[3].dstArrayElement = 0;
                descriptorComposition[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorComposition[3].descriptorCount = 1;
                descriptorComposition[3].pBufferInfo = m_ubos.uboLights.getDescriptorPointer();
                descriptorComposition[3].dstSet = m_descriptorSets.composition;
                vkUpdateDescriptorSets(
                        m_device.getLogicalDevice(), 
                        static_cast<uint32_t>(descriptorComposition.size()), 
                        descriptorComposition.data(), 
                        0, 
                        NULL);

                // Transparent
                allocInfo.pSetLayouts = m_descriptorSetLayouts.transparent.getDescriptorSetLayoutPointer();

                VK_CHECK_RESULT(vkAllocateDescriptorSets(
                            m_device.getLogicalDevice(), 
                            &allocInfo, 
                            &m_descriptorSets.transparent));

                std::vector<VkWriteDescriptorSet> descriptorTransparent(3);
                descriptorTransparent[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorTransparent[0].dstBinding = 0;
                descriptorTransparent[0].dstArrayElement = 0;
                descriptorTransparent[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorTransparent[0].descriptorCount = 1;
                descriptorTransparent[0].pBufferInfo = m_ubos.uboMatrices.getDescriptorPointer();
                descriptorTransparent[0].dstSet = m_descriptorSets.transparent;

                descriptorTransparent[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorTransparent[1].dstBinding = 1;
                descriptorTransparent[1].dstArrayElement = 0;
                descriptorTransparent[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                descriptorTransparent[1].descriptorCount = 1;
                descriptorTransparent[1].pImageInfo = &texDescriptorImageInfo[0];
                descriptorTransparent[1].dstSet = m_descriptorSets.transparent;

                descriptorTransparent[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorTransparent[2].dstBinding = 2;
                descriptorTransparent[2].dstArrayElement = 0;
                descriptorTransparent[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorTransparent[2].descriptorCount = 1;
                descriptorTransparent[2].pImageInfo = m_glassTexture.getDescriptorPointer();
                descriptorTransparent[2].dstSet = m_descriptorSets.transparent;

                vkUpdateDescriptorSets(
                        m_device.getLogicalDevice(), 
                        static_cast<uint32_t>(descriptorTransparent.size()), 
                        descriptorTransparent.data(), 
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

                m_uboLights.viewPos = glm::vec4(m_camera.position(), 0.0f);

                memcpy(m_ubos.uboMatrices.getMappedMemory(), &m_uboMatrices, sizeof(m_uboMatrices));
                memcpy(m_ubos.uboLights.getMappedMemory(), &m_uboLights, sizeof(m_uboLights));
            }

            void loadAssets() {
                uint32_t glTFLoadingFlags = FileLoadingFlags::PreTransformVertices | FileLoadingFlags::PreMultiplyVertexColors;
                m_models.scene.loadFromFile("src/models/samplebuilding.gltf", &m_device, m_device.getGraphicsQueue(), glTFLoadingFlags);
                m_models.transparent.loadFromFile("src/models/samplebuilding_glass.gltf", &m_device, m_device.getGraphicsQueue(), glTFLoadingFlags);
                m_glassTexture.loadFromKTXFile("src/textures/colored_glass_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, &m_device, m_device.getGraphicsQueue());
            }

            void OnUpdateUI (UI *ui) override {
                if (ui->header("Settings")) {
                    ui->text("Subpasses");
                    if (ui->header("Subpasses")) {
                        ui->text("0: Deferred G-Buffer creation");
                        ui->text("1: Deferred composition");
                        ui->text("2: Forward transparency");
                    }
                    if (ui->header("Settings")) {
                        if (ui->button("Randomize lights")) {
                            initLights();
                            updateUniformBuffers();
                        }
                    }
                }
            }

            void createFramebuffers() override {
                // might need to recreate attachments when resized
                VkImageView views[5];

                VkFramebufferCreateInfo framebufferCreateInfo = {};
                framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferCreateInfo.renderPass = m_renderPass.getRenderPass();
                framebufferCreateInfo.attachmentCount = 5;
                framebufferCreateInfo.pAttachments = views;
                framebufferCreateInfo.width = m_swapChain.getExtent().width;
                framebufferCreateInfo.height = m_swapChain.getExtent().height;
                framebufferCreateInfo.layers = 1;

                m_framebuffers.resize(m_swapChain.getImages().size());

                for (uint32_t i = 0; i < m_framebuffers.size(); i++) {
                    views[0] = m_swapChain.getImagesViews()[i];
                    views[1] = m_attachments.position.view;
                    views[2] = m_attachments.normal.view;
                    views[3] = m_attachments.albedo.view;
                    views[4] = m_depthImageResource.getImageView();
                    vkCreateFramebuffer(m_device.getLogicalDevice(), &framebufferCreateInfo, nullptr, &m_framebuffers[i]);
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

            void createGAttachments() {
                createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &m_attachments.position);
                createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &m_attachments.normal);
                createAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &m_attachments.albedo);
            }

            void initLights() {
                std::vector<glm::vec3> colors =
                {
                    glm::vec3(1.0f, 1.0f, 1.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec3(1.0f, 1.0f, 0.0f),
                };

                std::default_random_engine rndGen( (unsigned)time(nullptr));
                std::uniform_real_distribution<float> rndDist(-1.0f, 1.0f);
                std::uniform_int_distribution<uint32_t> rndCol(0, static_cast<uint32_t>(colors.size()-1));

                for (auto& light : m_uboLights.lights)
                {
                    light.position = glm::vec4(rndDist(rndGen) * 6.0f, 0.25f + std::abs(rndDist(rndGen)) * 4.0f, rndDist(rndGen) * 6.0f, 1.0f);
                    light.color = colors[rndCol(rndGen)];
                    light.radius = 1.0f + std::abs(rndDist(rndGen));
                }
            }
    };
}

VULKAN_EXAMPLE_MAIN()
