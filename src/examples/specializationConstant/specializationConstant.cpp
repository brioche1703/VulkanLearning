#define STB_IMAGE_IMPLEMENTATION

#include "VulkanBase.hpp"

namespace VulkanLearning {

    const std::string MODEL_PATH = "./src/models/sphere2.obj";
    const std::string TEXTURE_PATH = "./src/textures/sphere_texture.png";

    class VulkanExample : public VulkanBase {
        private:
            VulkanDescriptorSets m_descriptorSets;

            VulkanTexture2D m_texture;

            struct {
                VkPipeline phong;
                VkPipeline toon;
                VkPipeline textured;
            } m_pipelines;

            VkPipelineLayout m_pipelineLayout;

            uint32_t m_msaaSamples = 64;

            struct UBOLight {
                glm::vec4 lightPos = glm::vec4(0.0f, -2.0f, 1.0f, 0.0f);
            } uboLight;

            std::vector<VulkanBuffer> m_lightUniformBuffers;

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
                m_camera = Camera(glm::vec3(0.0f, 0.0f, 7.0f));
                m_fpsCounter = FpsCounter();
                m_input = Inputs(m_window.getWindow(), &m_camera, &m_fpsCounter, &m_ui);

                glfwSetKeyCallback(m_window.getWindow() , m_input.keyboard_callback);
                glfwSetScrollCallback(m_window.getWindow() , m_input.scroll_callback);
                glfwSetCursorPosCallback(m_window.getWindow() , m_input.mouse_callback);

                glfwSetInputMode(m_window.getWindow() , GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

                createColorResources();
                createDepthResources();
                createFramebuffers();
                createTexture();

                loadModel();

                createVertexBuffer();
                createIndexBuffer();
                createCoordinateSystemUniformBuffers();
                createLightUniformBuffers();
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
                        &m_syncObjects.getInFlightFences()[m_currentFrame],
                        VK_TRUE, UINT64_MAX);

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
                            &m_syncObjects.getImagesInFlight()[imageIndex], 
                            VK_TRUE, UINT64_MAX);
                }

                m_syncObjects.getImagesInFlight()[imageIndex] = m_syncObjects.getInFlightFences()[m_currentFrame];

                updateCamera(imageIndex);
                updateLight(imageIndex);

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                VkSemaphore waitSemaphore[] = { 
                    m_syncObjects.getImageAvailableSemaphores()[m_currentFrame] 
                };

                VkPipelineStageFlags waitStages[] = {
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                };

                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphore;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = m_commandBuffers[imageIndex].getCommandBufferPointer();

                VkSemaphore signalSemaphores[] = {
                    m_syncObjects.getRenderFinishedSemaphores()[m_currentFrame]
                };

                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                vkResetFences(m_device.getLogicalDevice(), 1, 
                        &m_syncObjects.getInFlightFences()[m_currentFrame]);

                if (vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, 
                            m_syncObjects.getInFlightFences()[m_currentFrame]) 
                        != VK_SUCCESS) {
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

                if (result == VK_ERROR_OUT_OF_DATE_KHR 
                        || result == VK_SUBOPTIMAL_KHR 
                        || framebufferResized) {
                    framebufferResized = false;
                    recreateSwapChain();
                } else if (result != VK_SUCCESS) {
                    throw std::runtime_error("Presentation of one image of the swap chain failed!");
                }

                m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            }

            void cleanup() override {
                cleanupSwapChain();

                m_texture.destroy();

                m_descriptorSetLayout.cleanup();

                m_vertexBuffer.cleanup();
                m_indexBuffer.cleanup();

                m_syncObjects.cleanup();
                m_ui.freeResources();

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

                createCoordinateSystemUniformBuffers();
                createLightUniformBuffers();

                createDescriptorPool();
                createDescriptorSets();
                createCommandBuffers();
                m_ui.resize(m_swapChain.getExtent().width, m_swapChain.getExtent().height);
            }

            void cleanupSwapChain() override {
                m_colorImageResource.cleanup();
                m_depthImageResource.cleanup();

                m_swapChain.cleanFramebuffers();

                vkFreeCommandBuffers(
                        m_device.getLogicalDevice(), 
                        m_device.getCommandPool(), 
                        static_cast<uint32_t>(
                           m_commandBuffers.size()), 
                        m_commandBuffers.data()->getCommandBufferPointer());

                vkDestroyPipeline(m_device.getLogicalDevice(), 
                        m_pipelines.phong, nullptr);
                vkDestroyPipeline(m_device.getLogicalDevice(), 
                        m_pipelines.toon, nullptr);
                vkDestroyPipeline(m_device.getLogicalDevice(), 
                        m_pipelines.textured, nullptr);

                vkDestroyPipelineLayout(m_device.getLogicalDevice(), 
                        m_pipelineLayout, nullptr);
                vkDestroyRenderPass(m_device.getLogicalDevice(), 
                        m_renderPass.getRenderPass(), nullptr);

                m_swapChain.destroyImageViews();
                vkDestroySwapchainKHR(m_device.getLogicalDevice(), m_swapChain.getSwapChain(), nullptr);

                for (size_t i = 0; i < m_swapChain.getImages().size(); i++) {
                    vkDestroyBuffer(m_device.getLogicalDevice(), 
                            m_coordinateSystemUniformBuffers[i].getBuffer(), 
                            nullptr);
                    vkFreeMemory(m_device.getLogicalDevice(), 
                            m_coordinateSystemUniformBuffers[i].getBufferMemory(), 
                            nullptr);

                    vkDestroyBuffer(m_device.getLogicalDevice(), 
                            m_lightUniformBuffers[i].getBuffer(), 
                            nullptr);
                    vkFreeMemory(m_device.getLogicalDevice(), 
                            m_lightUniformBuffers[i].getBufferMemory(), 
                            nullptr);
                }

                vkDestroyDescriptorPool(m_device.getLogicalDevice(), 
                        m_descriptorPool.getDescriptorPool(), nullptr);
            }

            void  createInstance() override {
                m_instance = new VulkanInstance(
                        "Specialization Constants",
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
                VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
                pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutInfo.setLayoutCount = 1;
                pipelineLayoutInfo.pSetLayouts = 
                    m_descriptorSetLayout.getDescriptorSetLayoutPointer();

                if (vkCreatePipelineLayout(m_device.getLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
                    throw std::runtime_error("Pipeline layout creation failed!");
                }

                VulkanShaderModule vertShaderModule = 
                    VulkanShaderModule("src/shaders/specializationConstantVert.spv", &m_device, VK_SHADER_STAGE_VERTEX_BIT);
                VulkanShaderModule fragShaderModule = 
                    VulkanShaderModule("src/shaders/specializationConstantFrag.spv", &m_device, VK_SHADER_STAGE_FRAGMENT_BIT);

                auto bindingDescription = VertexTextured::getBindingDescription();
                auto attributeDescriptions = VertexTextured::getAttributeDescriptions();

                VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
                vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
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

                VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
                vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
                vertShaderStageInfo.module = vertShaderModule.getModule();
                vertShaderStageInfo.pName = "main";

                VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
                fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                fragShaderStageInfo.module = fragShaderModule.getModule();
                fragShaderStageInfo.pName = "main";

                VkPipelineShaderStageCreateInfo shaderStages[] = {
                    vertShaderStageInfo, fragShaderStageInfo
                };

                VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
                inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                inputAssembly.flags = 0;
                inputAssembly.primitiveRestartEnable = VK_FALSE;

                VkPipelineViewportStateCreateInfo viewportState{};
                viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                viewportState.viewportCount = 1;
                viewportState.scissorCount = 1;
                viewportState.flags = 0;

                VkPipelineRasterizationStateCreateInfo rasterizer{};
                rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                rasterizer.depthClampEnable = VK_FALSE;
                rasterizer.rasterizerDiscardEnable = VK_FALSE;
                rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
                rasterizer.lineWidth = 1.0f;
                rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
                rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                rasterizer.depthBiasEnable = VK_FALSE;

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

                VkPipelineColorBlendAttachmentState colorBlendAttachment{};
                colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT 
                    | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT 
                    | VK_COLOR_COMPONENT_A_BIT;
                colorBlendAttachment.blendEnable = VK_FALSE;

                VkPipelineColorBlendStateCreateInfo colorBlending{};
                colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colorBlending.logicOpEnable = VK_FALSE;
                colorBlending.logicOp = VK_LOGIC_OP_COPY;
                colorBlending.attachmentCount = 1;
                colorBlending.pAttachments = &colorBlendAttachment;
                colorBlending.blendConstants[0] = 0.0f;
                colorBlending.blendConstants[1] = 0.0f;
                colorBlending.blendConstants[2] = 0.0f;
                colorBlending.blendConstants[3] = 0.0f;

                VkDynamicState dynamicStates[] = {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_SCISSOR,
                    VK_DYNAMIC_STATE_LINE_WIDTH
                };

                VkPipelineDynamicStateCreateInfo dynamicState{};
                dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                dynamicState.dynamicStateCount = 3;
                dynamicState.pDynamicStates = dynamicStates;

                VkGraphicsPipelineCreateInfo pipelineInfo{};
                pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                pipelineInfo.stageCount = 2;
                pipelineInfo.pStages = shaderStages;
                pipelineInfo.pVertexInputState = &vertexInputInfo;
                pipelineInfo.pInputAssemblyState = &inputAssembly;
                pipelineInfo.pViewportState = &viewportState;
                pipelineInfo.pRasterizationState = &rasterizer;
                pipelineInfo.pMultisampleState = &multisampling;
                pipelineInfo.pDepthStencilState = &depthStencil;
                pipelineInfo.pColorBlendState = &colorBlending;
                pipelineInfo.layout = m_pipelineLayout;
                pipelineInfo.renderPass = m_renderPass.getRenderPass();
                pipelineInfo.subpass = 0;
                pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
                pipelineInfo.pDynamicState = &dynamicState;

                struct SpecializationData {
                    uint32_t lightingModel;
                    float toonDesaturationFactor = 0.5f;
                } specializationData;

                std::array<VkSpecializationMapEntry, 2> specializationMapEntries;

                specializationMapEntries[0].constantID = 0;
                specializationMapEntries[0].size = sizeof(specializationData.lightingModel);
                specializationMapEntries[0].offset = 0;

                specializationMapEntries[1].constantID = 1;
                specializationMapEntries[1].size = sizeof(specializationData.toonDesaturationFactor);
                specializationMapEntries[1].offset = offsetof(SpecializationData, toonDesaturationFactor);
                
                VkSpecializationInfo specializationInfo{};
                specializationInfo.dataSize = sizeof(specializationData.toonDesaturationFactor);
                specializationInfo.mapEntryCount = static_cast<uint32_t>(specializationMapEntries.size());
                specializationInfo.pMapEntries = specializationMapEntries.data();
                specializationInfo.pData = &specializationData;

                shaderStages[1].pSpecializationInfo = &specializationInfo;

                // Phong
                specializationData.lightingModel = 0;
                if (vkCreateGraphicsPipelines(
                            m_device.getLogicalDevice(), 
                            VK_NULL_HANDLE, 
                            1, 
                            &pipelineInfo, 
                            nullptr, 
                            &m_pipelines.phong) != VK_SUCCESS) {
                    throw std::runtime_error("Graphics pipeline creation failed!");
                }

                // Toon
                specializationData.lightingModel = 1;
                if (vkCreateGraphicsPipelines(
                            m_device.getLogicalDevice(), 
                            VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, 
                            &m_pipelines.toon) != VK_SUCCESS) {
                    throw std::runtime_error("Graphics pipeline creation failed!");
                }

                // Textured
                specializationData.lightingModel = 2;
                if (vkCreateGraphicsPipelines(
                            m_device.getLogicalDevice(), 
                            VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, 
                            &m_pipelines.textured) != VK_SUCCESS) {
                    throw std::runtime_error("Graphics pipeline creation failed!");
                }

                vkDestroyShaderModule(m_device.getLogicalDevice(), vertShaderModule.getModule(), nullptr);
                vkDestroyShaderModule(m_device.getLogicalDevice(), fragShaderModule.getModule(), nullptr);
            }

            void createFramebuffers() override {
                const std::vector<VkImageView> attachments {
                    m_colorImageResource.getImageView(),
                    m_depthImageResource.getImageView()
                };

                m_swapChain.createFramebuffers(m_renderPass.getRenderPass(), 
                        attachments);
            }

            void createTexture() override {
                m_texture.loadFromFile(TEXTURE_PATH, VK_FORMAT_R8G8B8A8_SRGB, &m_device, m_device.getGraphicsQueue());
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

            void createVertexBuffer() override {
                m_vertexBuffer = VulkanBuffer(m_device);
                m_vertexBuffer.createWithStagingBuffer(m_model.getVerticies(), 
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            }

            void createIndexBuffer() override {
                m_indexBuffer = VulkanBuffer(m_device);
                m_indexBuffer.createWithStagingBuffer(m_model.getIndicies(), 
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
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 
                            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                            *m_coordinateSystemUniformBuffers[i].getBufferPointer(), 
                            *m_coordinateSystemUniformBuffers[i].getBufferMemoryPointer());
                }
            }

            void createLightUniformBuffers() {
                VkDeviceSize bufferSize = sizeof(UBOLight);

                m_lightUniformBuffers.resize(m_swapChain.getImages().size());

                for (size_t i = 0; i < m_swapChain.getImages().size(); i++) {
                    m_lightUniformBuffers[i] = VulkanBuffer(m_device);
                    m_lightUniformBuffers[i].createBuffer(
                            bufferSize, 
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 
                            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                            *m_lightUniformBuffers[i].getBufferPointer(), 
                            *m_lightUniformBuffers[i].getBufferMemoryPointer());
                }
            }

            void createCommandBuffers() override {

                m_commandBuffers.resize(m_swapChain.getFramebuffers().size());

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
                clearValues[1].depthStencil = {1.0f, 0};

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
                    renderPassInfo.framebuffer = m_swapChain.getFramebuffers()[i];
                    renderPassInfo.renderArea.offset = {0, 0};
                    renderPassInfo.renderArea.extent = m_swapChain.getExtent();

                    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
                    renderPassInfo.pClearValues = clearValues.data();

                    vkCmdBeginRenderPass(
                            m_commandBuffers[i].getCommandBuffer(), 
                            &renderPassInfo, 
                            VK_SUBPASS_CONTENTS_INLINE);

                    VkRect2D scissor{};
                    scissor.extent = m_swapChain.getExtent();
                    scissor.offset = {0, 0};
                    vkCmdSetScissor(m_commandBuffers[i].getCommandBuffer(), 0, 1, &scissor);

                    vkCmdBindDescriptorSets(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelineLayout, 
                            0, 
                            1, 
                            &m_descriptorSets.getDescriptorSets()[i], 
                            0, 
                            nullptr);

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

                    // Left Viewport
                    VkViewport viewport;
                    viewport.height = m_swapChain.getExtent().height;
                    viewport.width = m_swapChain.getExtent().width / 3.0f;
                    viewport.minDepth = 0.0f;
                    viewport.maxDepth = 1.0f;
                    viewport.x = 0.0f;
                    viewport.y = 0.0f;

                    vkCmdSetViewport(
                            m_commandBuffers[i].getCommandBuffer(), 
                            0,
                            1, 
                            &viewport);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelines.phong);

                    vkCmdDrawIndexed(
                            m_commandBuffers[i].getCommandBuffer(), 
                            static_cast<uint32_t>(m_model.getIndicies().size()), 
                            1, 
                            0, 
                            0, 
                            0);

                    // Middle Viewport
                    viewport.x = (float) m_swapChain.getExtent().width / 3.0f;
                    vkCmdSetViewport(
                            m_commandBuffers[i].getCommandBuffer(), 
                            0,
                            1, 
                            &viewport);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelines.toon);

                    vkCmdDrawIndexed(
                            m_commandBuffers[i].getCommandBuffer(), 
                            static_cast<uint32_t>(m_model.getIndicies().size()), 
                            1, 
                            0, 
                            0, 
                            0);

                    // Right Viewport
                    viewport.x = (float) m_swapChain.getExtent().width / 3.0f + (float) m_swapChain.getExtent().width / 3.0f;
                    vkCmdSetViewport(
                            m_commandBuffers[i].getCommandBuffer(), 
                            0,
                            1, 
                            &viewport);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelines.textured);

                    vkCmdDrawIndexed(
                            m_commandBuffers[i].getCommandBuffer(), 
                            static_cast<uint32_t>(m_model.getIndicies().size()), 
                            1, 
                            0, 
                            0, 
                            0);

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
                m_descriptorSetLayout = VulkanDescriptorSetLayout(m_device);

                VkDescriptorSetLayoutBinding uboLayoutBinding{};
                uboLayoutBinding.binding = 0;
                uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uboLayoutBinding.descriptorCount = 1;
                uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                uboLayoutBinding.pImmutableSamplers = nullptr;

                VkDescriptorSetLayoutBinding uboLightsLayoutBinding{};
                uboLightsLayoutBinding.binding = 2;
                uboLightsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uboLightsLayoutBinding.descriptorCount = 1;
                uboLightsLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                uboLightsLayoutBinding.pImmutableSamplers = nullptr;

                VkDescriptorSetLayoutBinding samplerLayoutBinding{};
                samplerLayoutBinding.binding = 1;
                samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                samplerLayoutBinding.descriptorCount = 1;
                samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                samplerLayoutBinding.pImmutableSamplers = nullptr;
        
                std::vector<VkDescriptorSetLayoutBinding> bindings = {
                    uboLayoutBinding, 
                    samplerLayoutBinding, 
                    uboLightsLayoutBinding
                };

                m_descriptorSetLayout.create(bindings);
            }

            void createDescriptorPool() override {
                m_descriptorPool = VulkanDescriptorPool(m_device, m_swapChain);

                std::vector<VkDescriptorPoolSize> poolSizes = std::vector<VkDescriptorPoolSize>(3);
                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = static_cast<uint32_t>(
                        m_swapChain.getImages().size());
                poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[1].descriptorCount = static_cast<uint32_t>(
                        m_swapChain.getImages().size());
                poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[2].descriptorCount = static_cast<uint32_t>(
                        m_swapChain.getImages().size());

                m_descriptorPool.create(poolSizes);
            }

            void createDescriptorSets() override {
                std::vector<std::vector<VulkanBuffer>> ubos {   
                    m_coordinateSystemUniformBuffers, 
                    m_lightUniformBuffers
                };

                std::vector<VkDeviceSize> ubosSizes {
                    sizeof(CoordinatesSystemUniformBufferObject),
                    sizeof(UBOLight)
                };

                m_descriptorSets = VulkanDescriptorSets(
                        m_device, 
                        m_descriptorSetLayout, 
                        m_descriptorPool);

                m_descriptorSets.create(static_cast<uint32_t>(m_swapChain.getImages().size()));
                
                for (size_t i = 0; i < m_swapChain.getImages().size(); i++) {
                    std::vector<VkWriteDescriptorSet> descriptorWrites = 
                        std::vector<VkWriteDescriptorSet>(3);

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

                    VkDescriptorImageInfo imageInfo{};
                    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfo.imageView = m_texture.getView();
                    imageInfo.sampler = m_texture.getSampler();

                    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrites[1].dstBinding = 1;
                    descriptorWrites[1].dstArrayElement = 0;
                    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    descriptorWrites[1].descriptorCount = 1;
                    descriptorWrites[1].pImageInfo = &imageInfo;

                    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrites[2].dstBinding = 2;
                    descriptorWrites[2].dstArrayElement = 0;
                    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    descriptorWrites[2].descriptorCount = 1;
                    descriptorWrites[2].pBufferInfo = &buffersInfo[1];

                    m_descriptorSets.update(descriptorWrites, i);
                }
            }

            void updateCamera(uint32_t currentImage) override {
                CoordinatesSystemUniformBufferObject ubo{};

                ubo.model = glm::mat4(1.0f);

                ubo.view = m_camera.getViewMatrix();

                ubo.proj = glm::perspective(glm::radians(m_camera.getZoom()), 
                        m_swapChain.getExtent().width / 3.0f / (float) m_swapChain.getExtent().height, 
                        0.1f,  100.0f);

                ubo.proj[1][1] *= -1;

                ubo.camPos = m_camera.position();

                m_coordinateSystemUniformBuffers[currentImage].map();
                memcpy(m_coordinateSystemUniformBuffers[currentImage].getMappedMemory(),
                        &ubo, sizeof(ubo));
                m_coordinateSystemUniformBuffers[currentImage].unmap();
            }

            void updateLight(uint32_t currentImage) {
                UBOLight ubo{};

                ubo.lightPos = glm::vec4(5.0f, 0.0f, 5.0f, 0.0f);

                m_lightUniformBuffers[currentImage].map();
                memcpy(m_lightUniformBuffers[currentImage].getMappedMemory(),
                        &ubo, sizeof(ubo));
                m_lightUniformBuffers[currentImage].unmap();
            }

            void loadModel() override {
                m_model = ModelObj(MODEL_PATH);
            }
    };

}

VULKAN_EXAMPLE_MAIN()
