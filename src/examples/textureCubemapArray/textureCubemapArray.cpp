#define TINYGLTF_IMPLEMENTATION

#include "VulkanBase.hpp"

#include "VulkanglTFModel.hpp"

namespace VulkanLearning {

    class VulkanExample : public VulkanBase {

        private:
            uint32_t m_msaaSamples = 64;

            VkPipelineLayout m_pipelineLayout;

            bool m_displaySkybox = true;
            Texture m_cubeMapTextureArray;

            struct Models {
                VulkanglTFModel skybox;
                std::vector<VulkanglTFModel> objects;
                int32_t index = 0;
            } m_models;

            struct {
                VulkanBuffer skybox;
                VulkanBuffer object;
            } m_uniformBuffers;

            struct UBOVS {
                glm::mat4 projection;
                glm::mat4 modelView;
                glm::mat4 inverseModelView;
                float lodBias = 0.0f;
                int32_t cubeMapIndex = 1;
            } m_uboVS;

            struct {
                VkPipeline skybox;
                VkPipeline reflect;
            } m_pipelines;

            struct {
                VkDescriptorSet object;
                VkDescriptorSet skybox;
            } m_descriptorSets;

            VulkanDescriptorSetLayout m_descriptorSetLayout;
            std::vector<std::string> m_objectNames;

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
                glfwSetCursorPosCallback(m_window.getWindow() , m_input.mouse_callback);
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
                vkWaitForFences(
                        m_device.getLogicalDevice(), 
                        1, 
                        &m_syncObjects.getInFlightFences()[currentFrame], 
                        VK_TRUE, 
                        UINT64_MAX);

                uint32_t imageIndex;

                VkResult result = vkAcquireNextImageKHR(
                        m_device.getLogicalDevice(), 
                        m_swapChain.getSwapChain(), 
                        UINT64_MAX, 
                        m_syncObjects.getImageAvailableSemaphores()[currentFrame], 
                        VK_NULL_HANDLE, 
                        &imageIndex);

                if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                    recreateSwapChain();
                    return;
                } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                    throw std::runtime_error("Presentation of one image of the swap chain failed!");
                }

                if (m_syncObjects.getImagesInFlight()[imageIndex] != VK_NULL_HANDLE) {
                    vkWaitForFences(
                            m_device.getLogicalDevice(), 
                            1, 
                            &m_syncObjects.getImagesInFlight()[imageIndex], 
                            VK_TRUE, 
                            UINT64_MAX);
                }

                m_syncObjects.getImagesInFlight()[imageIndex] = 
                    m_syncObjects.getInFlightFences()[currentFrame];

                updateUniformBuffers();

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                VkSemaphore waitSemaphore[] = {
                    m_syncObjects.getImageAvailableSemaphores()[currentFrame]
                };

                VkPipelineStageFlags waitStages[] = {
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                };

                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphore;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = 
                    m_commandBuffers[imageIndex].getCommandBufferPointer();

                VkSemaphore signalSemaphores[] = {
                    m_syncObjects.getRenderFinishedSemaphores()[currentFrame]
                };

                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                vkResetFences(
                        m_device.getLogicalDevice(), 
                        1, 
                        &m_syncObjects.getInFlightFences()[currentFrame]);

                if (vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, m_syncObjects.getInFlightFences()[currentFrame]) != VK_SUCCESS) {
                    throw std::runtime_error("Command buffer sending failed!");
                }

                VkSwapchainKHR swapChains[] = {
                    m_swapChain.getSwapChain()
                };

                VkPresentInfoKHR presentInfo{};
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = signalSemaphores;
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

                currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            }

            void cleanup() override {
                cleanupSwapChain();

                m_cubeMapTextureArray.destroy();

                m_descriptorSetLayout.cleanup();

                for (size_t i = 0; i < m_models.objects.size(); i++) {
                    m_models.objects[i].~VulkanglTFModel();
                }

                m_models.skybox.~VulkanglTFModel();

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

                vkFreeCommandBuffers(
                        m_device.getLogicalDevice(), 
                        m_device.getCommandPool(), 
                        static_cast<uint32_t>(m_commandBuffers.size()), 
                        m_commandBuffers.data()->getCommandBufferPointer());

                vkDestroyPipeline(m_device.getLogicalDevice(), m_pipelines.reflect, nullptr);
                vkDestroyPipeline(m_device.getLogicalDevice(), m_pipelines.skybox, nullptr);

                vkDestroyPipelineLayout(m_device.getLogicalDevice(), m_pipelineLayout, nullptr);
                vkDestroyRenderPass(m_device.getLogicalDevice(), m_renderPass.getRenderPass(), nullptr);

                m_swapChain.destroyImageViews();
                vkDestroySwapchainKHR(m_device.getLogicalDevice(), m_swapChain.getSwapChain(), nullptr);

                m_uniformBuffers.object.cleanup();
                m_uniformBuffers.skybox.cleanup();

                vkDestroyDescriptorPool(
                        m_device.getLogicalDevice(), 
                        m_descriptorPool.getDescriptorPool(), 
                        nullptr);
            }

            void  createInstance() override {
                m_instance = new VulkanInstance(
                        "Texture Cubemap",
                        enableValidationLayers, 
                        validationLayers, 
                        m_debug);
            }

            void  createDebug() override {
                m_debug = new VulkanDebug(m_instance->getInstance(), 
                        enableValidationLayers);
            }

            void checkAndEnableFeatures() override {
                if (m_device.features.imageCubeArray) {
                    m_device.enabledFeatures.imageCubeArray = VK_TRUE;
                } else {
                    std::runtime_error("Cubemap array textures not supported by GPU");
                }
            }

            void  createDevice() override {
                m_device = VulkanDevice(m_msaaSamples);
                m_device.pickPhysicalDevice(m_instance->getInstance(), m_surface.getSurface(), deviceExtensions);
                checkAndEnableFeatures();
                m_device.createLogicalDevice(m_surface.getSurface(), enableValidationLayers, validationLayers);
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

                VulkanShaderModule vertShaderModuleReflect = 
                    VulkanShaderModule(
                            "src/shaders/textureCubemapArray/reflectVert.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_VERTEX_BIT);
                VulkanShaderModule fragShaderModuleReflect = 
                    VulkanShaderModule(
                            "src/shaders/textureCubemapArray/reflectFrag.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_FRAGMENT_BIT);

                VulkanShaderModule vertShaderModuleSkybox = 
                    VulkanShaderModule(
                            "src/shaders/textureCubemapArray/skyboxVert.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_VERTEX_BIT);
                VulkanShaderModule fragShaderModuleSkybox = 
                    VulkanShaderModule(
                            "src/shaders/textureCubemapArray/skyboxFrag.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_FRAGMENT_BIT);


                VkPipelineShaderStageCreateInfo shaderStagesSkybox[] = {
                    vertShaderModuleSkybox.getStageCreateInfo(), 
                    fragShaderModuleSkybox.getStageCreateInfo()
                };
                VkPipelineShaderStageCreateInfo shaderStagesReflect[] = {
                    vertShaderModuleReflect.getStageCreateInfo(), 
                    fragShaderModuleReflect.getStageCreateInfo()
                };

                VkGraphicsPipelineCreateInfo pipelineInfo{};
                pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                pipelineInfo.stageCount = 2;
                pipelineInfo.pStages = shaderStagesSkybox;
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
                pipelineInfo.pVertexInputState = Vertex::getPipelineVertexInputState({
                        VertexComponent::Position});

                rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
                VK_CHECK_RESULT(vkCreateGraphicsPipelines(
                            m_device.getLogicalDevice(), 
                            VK_NULL_HANDLE, 
                            1, 
                            &pipelineInfo, 
                            nullptr, 
                            &m_pipelines.skybox));

                rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
                depthStencilState.depthWriteEnable = VK_TRUE;
                depthStencilState.depthTestEnable = VK_TRUE;
                pipelineInfo.pStages = shaderStagesReflect;
                pipelineInfo.pVertexInputState = Vertex::getPipelineVertexInputState({
                        VertexComponent::Position,
                        VertexComponent::Normal});

                VK_CHECK_RESULT(vkCreateGraphicsPipelines(
                            m_device.getLogicalDevice(), 
                            VK_NULL_HANDLE, 
                            1, 
                            &pipelineInfo, 
                            nullptr, 
                            &m_pipelines.reflect));

                vkDestroyShaderModule(
                        m_device.getLogicalDevice(), 
                        vertShaderModuleReflect.getModule(), 
                        nullptr);
                vkDestroyShaderModule(
                        m_device.getLogicalDevice(), 
                        fragShaderModuleReflect.getModule(), 
                        nullptr);
                vkDestroyShaderModule(
                        m_device.getLogicalDevice(), 
                        vertShaderModuleSkybox.getModule(), 
                        nullptr);
                vkDestroyShaderModule(
                        m_device.getLogicalDevice(), 
                        fragShaderModuleSkybox.getModule(), 
                        nullptr);
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
                m_colorImageResource = VulkanImageResource(
                        m_device, 
                        m_swapChain, 
                        m_swapChain.getImageFormat(),  
                        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
                        VK_IMAGE_ASPECT_COLOR_BIT);
                m_colorImageResource.create();
            }

            void createDepthResources() override {
                m_depthImageResource = VulkanImageResource(
                        m_device, 
                        m_swapChain,
                        m_device.findDepthFormat(), 
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                        VK_IMAGE_ASPECT_DEPTH_BIT);
                m_depthImageResource.create();
            }

            void createUniformBuffers() {
                m_uniformBuffers.object = VulkanBuffer(m_device);
                m_uniformBuffers.skybox = VulkanBuffer(m_device);

                m_uniformBuffers.object.createBuffer(
                        sizeof(m_uboVS), 
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                m_uniformBuffers.skybox.createBuffer(
                        sizeof(m_uboVS), 
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                m_uniformBuffers.object.map();
                m_uniformBuffers.skybox.map();

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

                    if (m_displaySkybox) {
                        vkCmdBindDescriptorSets(
                                m_commandBuffers[i].getCommandBuffer(), 
                                VK_PIPELINE_BIND_POINT_GRAPHICS, 
                                m_pipelineLayout,
                                0, 
                                1, 
                                &m_descriptorSets.skybox,
                                0, 
                                nullptr);

                        vkCmdBindPipeline(
                                m_commandBuffers[i].getCommandBuffer(), 
                                VK_PIPELINE_BIND_POINT_GRAPHICS, 
                                m_pipelines.skybox);

                        m_models.skybox.draw(m_commandBuffers[i].getCommandBuffer());
                    }

                    vkCmdBindDescriptorSets(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelineLayout,
                            0, 
                            1, 
                            &m_descriptorSets.object,
                            0, 
                            nullptr);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelines.reflect);

                    m_models.objects[m_models.index].draw(m_commandBuffers[i].getCommandBuffer());
                    
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

                std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings(2);
                descriptorSetLayoutBindings[0].binding = 0;
                descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorSetLayoutBindings[0].descriptorCount = 1;
                descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                descriptorSetLayoutBindings[0].pImmutableSamplers = nullptr;

                descriptorSetLayoutBindings[1].binding = 1;
                descriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorSetLayoutBindings[1].descriptorCount = 1;
                descriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                descriptorSetLayoutBindings[1].pImmutableSamplers = nullptr;

                m_descriptorSetLayout.create(descriptorSetLayoutBindings);
            }

            void createDescriptorPool() override {
                m_descriptorPool = VulkanDescriptorPool(m_device, m_swapChain);

                std::vector<VkDescriptorPoolSize> poolSizes = std::vector<VkDescriptorPoolSize>(2);

                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = 2;
                poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[1].descriptorCount = 2;

                m_descriptorPool.create(poolSizes);
            }

            void createDescriptorSets() override {
                // Object
                VkDescriptorSetAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool = m_descriptorPool.getDescriptorPool();
                allocInfo.descriptorSetCount = 1;
                allocInfo.pSetLayouts = m_descriptorSetLayout.getDescriptorSetLayoutPointer();

                VK_CHECK_RESULT(vkAllocateDescriptorSets(
                            m_device.getLogicalDevice(), 
                            &allocInfo, 
                            &m_descriptorSets.object));

                VkDescriptorImageInfo textureDescriptor = {};
                textureDescriptor.imageView = m_cubeMapTextureArray.view;
                textureDescriptor.imageLayout = m_cubeMapTextureArray.imageLayout;
                textureDescriptor.sampler = m_cubeMapTextureArray.sampler;

                std::vector<VkWriteDescriptorSet> descriptorWrites(2);

                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[0].descriptorCount = 1;
                descriptorWrites[0].pBufferInfo = m_uniformBuffers.object.getDescriptorPointer();
                descriptorWrites[0].dstSet = m_descriptorSets.object;

                descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[1].dstBinding = 1;
                descriptorWrites[1].dstArrayElement = 0;
                descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[1].descriptorCount = 1;
                descriptorWrites[1].pImageInfo = &textureDescriptor;
                descriptorWrites[1].dstSet = m_descriptorSets.object;

                vkUpdateDescriptorSets(m_device.getLogicalDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, NULL);

                // Skybox
                VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device.getLogicalDevice(), &allocInfo, &m_descriptorSets.skybox));
                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[0].descriptorCount = 1;
                descriptorWrites[0].pBufferInfo = m_uniformBuffers.skybox.getDescriptorPointer();
                descriptorWrites[0].dstSet = m_descriptorSets.skybox;

                descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[1].dstBinding = 1;
                descriptorWrites[1].dstArrayElement = 0;
                descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[1].descriptorCount = 1;
                descriptorWrites[1].pImageInfo = &textureDescriptor;
                descriptorWrites[1].dstSet = m_descriptorSets.skybox;

                vkUpdateDescriptorSets(m_device.getLogicalDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, NULL);
            }

            void updateUniformBuffers() {
                m_uboVS.projection = glm::perspective(
                        glm::radians(m_camera.getZoom()), 
                        m_swapChain.getExtent().width / (float) m_swapChain.getExtent().height, 
                        0.1f,  100.0f);
                m_uboVS.projection[1][1] *= -1;

                m_uboVS.modelView = m_camera.getViewMatrix();
                m_uboVS.inverseModelView = glm::inverse(m_camera.getViewMatrix());

                memcpy(m_uniformBuffers.object.getMappedMemory(), &m_uboVS, sizeof(m_uboVS));

                m_uboVS.modelView[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                memcpy(m_uniformBuffers.skybox.getMappedMemory(), &m_uboVS, sizeof(m_uboVS));
            }

            void loadAssets() {
                m_objectNames = { "Sphere", "Teapot", "Torus", "Venus" };
                std::vector<std::string> fileNames = {
                    "src/models/sphere.gltf", 
                    "src/models/teapot.gltf",
                    "src/models/torusknot.gltf",
                    "src/models/venus.gltf",
                };

                uint32_t glTFLoadingFlags = FileLoadingFlags::PreTransformVertices;
                m_models.objects.resize(m_objectNames.size());
                for (size_t i = 0; i < fileNames.size(); i++) {
                    m_models.objects[i].loadFromFile(fileNames[i].c_str(), &m_device, m_device.getGraphicsQueue(), glTFLoadingFlags);
                }

                m_models.skybox.loadFromFile("src/models/cube.gltf", &m_device, m_device.getGraphicsQueue(), glTFLoadingFlags);

                loadCubemapTextureArray("src/textures/cubemap_array.ktx", VK_FORMAT_R8G8B8A8_UNORM, true);
            }

            void loadCubemapTextureArray(std::string filename, VkFormat format, bool forceLinearTiling) {
                ktxResult result;
                ktxTexture* ktxTexture;

                if (!tools::fileExists(filename)) {
                    tools::exitFatal("Could not load texture from " + filename, -1);
                }
               
                result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
                assert(result == KTX_SUCCESS);

                m_cubeMapTextureArray.device = &m_device;
                m_cubeMapTextureArray.width = ktxTexture->baseWidth;
                m_cubeMapTextureArray.height = ktxTexture->baseHeight;
                m_cubeMapTextureArray.mipLevels = ktxTexture->numLevels;
                m_cubeMapTextureArray.layerCount = ktxTexture->numLayers;

                ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);
                ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

                VulkanBuffer stagingBuffer = VulkanBuffer(m_device);
                stagingBuffer.createBuffer(
                        ktxTextureSize, 
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                uint8_t* data;
                vkMapMemory(m_device.getLogicalDevice(), stagingBuffer.getBufferMemory(), 0, stagingBuffer.getMemoryRequirements()->size, 0, (void **)&data);
                memcpy(data, ktxTextureData, ktxTextureSize);
                vkUnmapMemory(m_device.getLogicalDevice(), stagingBuffer.getBufferMemory());

                VkImageCreateInfo imageCreateInfo = {};
                imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
                imageCreateInfo.format = format;
                imageCreateInfo.mipLevels = m_cubeMapTextureArray.mipLevels;
                imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageCreateInfo.extent = { m_cubeMapTextureArray.width, m_cubeMapTextureArray.height, 1};
                imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                imageCreateInfo.arrayLayers = 6 * m_cubeMapTextureArray.layerCount;
                imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

                VK_CHECK_RESULT(vkCreateImage(m_device.getLogicalDevice(), &imageCreateInfo, nullptr, &m_cubeMapTextureArray.image));

                VkMemoryRequirements memReqs;
                vkGetImageMemoryRequirements(m_device.getLogicalDevice(), m_cubeMapTextureArray.image, &memReqs);

                VkMemoryAllocateInfo memAllocInfo = {};
                memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                memAllocInfo.allocationSize = memReqs.size;
                memAllocInfo.memoryTypeIndex = m_device.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                VK_CHECK_RESULT(vkAllocateMemory(m_device.getLogicalDevice(), &memAllocInfo, nullptr, &m_cubeMapTextureArray.deviceMemory));
                VK_CHECK_RESULT(vkBindImageMemory(m_device.getLogicalDevice(), m_cubeMapTextureArray.image, m_cubeMapTextureArray.deviceMemory, 0));

                VulkanCommandBuffer copyCmd;
                copyCmd.create(&m_device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

                std::vector<VkBufferImageCopy> bufferCopyRegions;
                uint32_t offset = 0;

                for (uint32_t face = 0; face < 6; face++) {
                    for (uint32_t layer = 0; layer < ktxTexture->numLayers; layer++) {
                        for (uint32_t level = 0; level < ktxTexture->numLevels; level++) {
                            ktx_size_t offset;
                            KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, level, layer, face, &offset);
                            assert(ret == KTX_SUCCESS);

                            VkBufferImageCopy bufferCopyRegion = {};
                            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                            bufferCopyRegion.imageSubresource.mipLevel = level;
                            bufferCopyRegion.imageSubresource.baseArrayLayer = layer * 6 + face;
                            bufferCopyRegion.imageSubresource.layerCount = 1;
                            bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> level;
                            bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> level;
                            bufferCopyRegion.imageExtent.depth = 1;
                            bufferCopyRegion.bufferOffset = offset;
                            bufferCopyRegions.push_back(bufferCopyRegion);
                        }
                    }
                }

                VkImageSubresourceRange subResourceRange = {};
                subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                subResourceRange.baseMipLevel = 0;
                subResourceRange.levelCount = m_cubeMapTextureArray.mipLevels;
                subResourceRange.layerCount = 6 * m_cubeMapTextureArray.layerCount;

                tools::setImageLayout(
                        copyCmd.getCommandBuffer(), 
                        m_cubeMapTextureArray.image, 
                        VK_IMAGE_LAYOUT_UNDEFINED, 
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                        subResourceRange);

                vkCmdCopyBufferToImage(
                        copyCmd.getCommandBuffer(), 
                        stagingBuffer.getBuffer(), 
                        m_cubeMapTextureArray.image, 
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                        static_cast<uint32_t>(bufferCopyRegions.size()), 
                        bufferCopyRegions.data());
               
                m_cubeMapTextureArray.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                tools::setImageLayout(
                        copyCmd.getCommandBuffer(), 
                        m_cubeMapTextureArray.image, 
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  
                        m_cubeMapTextureArray.imageLayout, 
                        subResourceRange);

                copyCmd.flushCommandBuffer(&m_device, m_device.getGraphicsQueue(), true);

                VkSamplerCreateInfo sampler = {};
                sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                sampler.magFilter = VK_FILTER_LINEAR;
                sampler.minFilter = VK_FILTER_LINEAR;
                sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                sampler.addressModeV = sampler.addressModeU;
                sampler.addressModeW = sampler.addressModeU;
                sampler.mipLodBias = 0.0f;
                sampler.compareOp = VK_COMPARE_OP_NEVER;
                sampler.minLod = 0.0f;
                sampler.maxLod = m_cubeMapTextureArray.mipLevels;
                sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
                sampler.maxAnisotropy = 1.0f;
                if (m_device.features.samplerAnisotropy)
                {
                    sampler.maxAnisotropy = m_device.properties.limits.maxSamplerAnisotropy;
                    sampler.anisotropyEnable = VK_TRUE;
                }
                VK_CHECK_RESULT(vkCreateSampler(m_device.getLogicalDevice(), &sampler, nullptr, &m_cubeMapTextureArray.sampler));

                VkImageViewCreateInfo imageView = {};
                imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                imageView.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
                imageView.format = format;
                imageView.components = {
                    VK_COMPONENT_SWIZZLE_R,
                    VK_COMPONENT_SWIZZLE_G,
                    VK_COMPONENT_SWIZZLE_B,
                    VK_COMPONENT_SWIZZLE_A
                };
                imageView.subresourceRange = {
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    0, 1, 0, 1
                };
                imageView.subresourceRange.layerCount = 6 * m_cubeMapTextureArray.layerCount;
                imageView.subresourceRange.levelCount = m_cubeMapTextureArray.mipLevels;
                imageView.image = m_cubeMapTextureArray.image;
                VK_CHECK_RESULT(vkCreateImageView(m_device.getLogicalDevice(), &imageView, nullptr, &m_cubeMapTextureArray.view));

                stagingBuffer.cleanup();
                ktxTexture_Destroy(ktxTexture);
            }

            void OnUpdateUI (UI *ui) override {
                if (ui->header("Settings")) {
                    if (ui->sliderInt("Cube map", &m_uboVS.cubeMapIndex, 0, m_cubeMapTextureArray.layerCount - 1)) {
                        updateUniformBuffers();
                    }
                    if (ui->sliderFloat("LOD bias", &m_uboVS.lodBias, 0.0f, (float)m_cubeMapTextureArray.mipLevels)) {
                        updateUniformBuffers();
                    }
                    if (ui->comboBox("Object type", &m_models.index, m_objectNames)) {
                        createCommandBuffers();
                    }
                    if (ui->checkBox("Skybox", &m_displaySkybox)) {
                        createCommandBuffers();
                    }
                }
            }

    };
}

VULKAN_EXAMPLE_MAIN()
