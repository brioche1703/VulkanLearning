#define TINYGLTF_IMPLEMENTATION
#include "VulkanBase.hpp"
#include "VulkanglTFSimpleModel.hpp"

namespace VulkanLearning {

    class VulkanExample : public VulkanBase {

        private:
            VulkanglTFSimpleModel glTFModel;

            VulkanDescriptorSets m_descriptorSets;

            VkPipeline m_pipeline;
            VkPipelineLayout m_pipelineLayout;

            bool m_wireframe = false;
            VkPipeline m_wireframePipeline = VK_NULL_HANDLE;

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

            ~VulkanExample() {
                cleanupSwapChain();

                m_descriptorSetLayouts.matrices->cleanup();
                m_descriptorSetLayouts.textures->cleanup();
            }

            void run() {
                VulkanBase::run();
            }

        private:

            void initVulkan() override {
                m_msaaSamples = 64;
                VulkanBase::initVulkan();

                m_window.setTitle("glTF Loading");
                m_camera.setPosition(glm::vec3(0.0f, 0.0f, -1.5f));
                m_camera.setYaw(90);

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

            void createPipelines() override {
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

                VulkanShaderModule vertShaderModule = 
                    VulkanShaderModule(
                            "src/shaders/glTFLoadingVert.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_VERTEX_BIT);

                VulkanShaderModule fragShaderModule = 
                    VulkanShaderModule(
                            "src/shaders/glTFLoadingFrag.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_FRAGMENT_BIT);

                VkPipelineShaderStageCreateInfo shaderStages[] = {
                    vertShaderModule.getStageCreateInfo(), 
                    fragShaderModule.getStageCreateInfo()
                };

                VkVertexInputBindingDescription vertexInputBindingDescription = {};
                vertexInputBindingDescription.binding = 0;
                vertexInputBindingDescription.stride = sizeof(VulkanglTFSimpleModel::Vertex);
                vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                std::vector<VkVertexInputBindingDescription> vertexInputBindingsDescription = {
                    vertexInputBindingDescription
                };
                
                std::vector<VkVertexInputAttributeDescription> vertexAttributeDescription;
                vertexAttributeDescription.resize(4);
                vertexAttributeDescription[0].binding = 0;
                vertexAttributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
                vertexAttributeDescription[0].location = 0;
                vertexAttributeDescription[0].offset = offsetof(VulkanglTFSimpleModel::Vertex, pos);

                vertexAttributeDescription[1].binding = 0;
                vertexAttributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
                vertexAttributeDescription[1].location = 1;
                vertexAttributeDescription[1].offset = offsetof(VulkanglTFSimpleModel::Vertex, normal);

                vertexAttributeDescription[2].binding = 0;
                vertexAttributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
                vertexAttributeDescription[2].location = 2;
                vertexAttributeDescription[2].offset = offsetof(VulkanglTFSimpleModel::Vertex, uv);

                vertexAttributeDescription[3].binding = 0;
                vertexAttributeDescription[3].format = VK_FORMAT_R32G32B32_SFLOAT;
                vertexAttributeDescription[3].location = 3;
                vertexAttributeDescription[3].offset = offsetof(VulkanglTFSimpleModel::Vertex, color);

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

                vkCreatePipelineLayout(
                        m_device.getLogicalDevice(), 
                        &pipelineLayoutInfo, 
                        nullptr, 
                        &m_pipelineLayout);

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

                rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
                rasterizer.lineWidth = 1.0f;
                VK_CHECK_RESULT(vkCreateGraphicsPipelines(
                            m_device.getLogicalDevice(), 
                            VK_NULL_HANDLE, 
                            1, 
                            &pipelineInfo, 
                            nullptr, 
                            &m_wireframePipeline));

                vkDestroyShaderModule(
                        m_device.getLogicalDevice(), 
                        vertShaderModule.getModule(), 
                        nullptr);
                vkDestroyShaderModule(
                        m_device.getLogicalDevice(), 
                        fragShaderModule.getModule(), 
                        nullptr);
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
                    renderPassBeginInfo.framebuffer = m_framebuffers[i];

                    vkCmdBeginRenderPass(m_commandBuffers[i].getCommandBuffer(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
                    vkCmdSetViewport(m_commandBuffers[i].getCommandBuffer(), 0, 1, &viewport);
                    vkCmdSetScissor(m_commandBuffers[i].getCommandBuffer(), 0, 1, &scissor);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipeline);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_wireframe ? m_wireframePipeline : m_pipeline);

                    vkCmdBindDescriptorSets(m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelineLayout,
                            0, 
                            1, 
                            &m_descriptorSets.getDescriptorSets()[i], 
                            0, 
                            nullptr);


                    glTFModel.draw(m_commandBuffers[i].getCommandBuffer(), m_pipelineLayout);

                    drawUI(m_commandBuffers[i].getCommandBuffer());

                    vkCmdEndRenderPass(m_commandBuffers[i].getCommandBuffer());
                    if (vkEndCommandBuffer(m_commandBuffers[i].getCommandBuffer()) != VK_SUCCESS) {
                        throw std::runtime_error("Recording of a command buffer failed!");
                    }
                }
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
                std::vector<VulkanglTFSimpleModel::Vertex> vertexBuffer;

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

                size_t vertexBufferSize = vertexBuffer.size() * sizeof(VulkanglTFSimpleModel::Vertex);
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
