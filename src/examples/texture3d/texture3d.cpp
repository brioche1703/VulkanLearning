#include "VulkanBase.hpp"

#include "VulkanglTFModel.hpp"

#include <numeric>
#include <random>


// Translation of Ken Perlin's JAVA implementation (http://mrl.nyu.edu/~perlin/noise/)
template <typename T>
class PerlinNoise
{
private:
	uint32_t permutations[512];
	T fade(T t)
	{
		return t * t * t * (t * (t * (T)6 - (T)15) + (T)10);
	}
	T lerp(T t, T a, T b)
	{
		return a + t * (b - a);
	}
	T grad(int hash, T x, T y, T z)
	{
		// Convert LO 4 bits of hash code into 12 gradient directions
		int h = hash & 15;
		T u = h < 8 ? x : y;
		T v = h < 4 ? y : h == 12 || h == 14 ? x : z;
		return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
	}
public:
	PerlinNoise()
	{
		// Generate random lookup for permutations containing all numbers from 0..255
		std::vector<uint8_t> plookup;
		plookup.resize(256);
		std::iota(plookup.begin(), plookup.end(), 0);
		std::default_random_engine rndEngine(std::random_device{}());
		std::shuffle(plookup.begin(), plookup.end(), rndEngine);

		for (uint32_t i = 0; i < 256; i++)
		{
			permutations[i] = permutations[256 + i] = plookup[i];
		}
	}
	T noise(T x, T y, T z)
	{
		// Find unit cube that contains point
		int32_t X = (int32_t)floor(x) & 255;
		int32_t Y = (int32_t)floor(y) & 255;
		int32_t Z = (int32_t)floor(z) & 255;
		// Find relative x,y,z of point in cube
		x -= floor(x);
		y -= floor(y);
		z -= floor(z);

		// Compute fade curves for each of x,y,z
		T u = fade(x);
		T v = fade(y);
		T w = fade(z);

		// Hash coordinates of the 8 cube corners
		uint32_t A = permutations[X] + Y;
		uint32_t AA = permutations[A] + Z;
		uint32_t AB = permutations[A + 1] + Z;
		uint32_t B = permutations[X + 1] + Y;
		uint32_t BA = permutations[B] + Z;
		uint32_t BB = permutations[B + 1] + Z;

		// And add blended results for 8 corners of the cube;
		T res = lerp(w, lerp(v,
			lerp(u, grad(permutations[AA], x, y, z), grad(permutations[BA], x - 1, y, z)), lerp(u, grad(permutations[AB], x, y - 1, z), grad(permutations[BB], x - 1, y - 1, z))),
			lerp(v, lerp(u, grad(permutations[AA + 1], x, y, z - 1), grad(permutations[BA + 1], x - 1, y, z - 1)), lerp(u, grad(permutations[AB + 1], x, y - 1, z - 1), grad(permutations[BB + 1], x - 1, y - 1, z - 1))));
		return res;
	}
};

// Fractal noise generator based on perlin noise above
template <typename T>
class FractalNoise
{
private:
	PerlinNoise<float> perlinNoise;
	uint32_t octaves;
	T frequency;
	T amplitude;
	T persistence;
public:

	FractalNoise(const PerlinNoise<T> &perlinNoise)
	{
		this->perlinNoise = perlinNoise;
		octaves = 6;
		persistence = (T)0.5;
	}

	T noise(T x, T y, T z)
	{
		T sum = 0;
		T frequency = (T)1;
		T amplitude = (T)1;
		T max = (T)0;
		for (uint32_t i = 0; i < octaves; i++)
		{
			sum += perlinNoise.noise(x * frequency, y * frequency, z * frequency) * amplitude;
			max += amplitude;
			amplitude *= persistence;
			frequency *= (T)2;
		}

		sum = sum / max;
		return (sum + (T)1.0) / (T)2.0;
	}
};

namespace VulkanLearning {

    class VulkanExample : public VulkanBase {

        private:
        struct Vertex {
            float pos[3];
            float uv[2];
            float normal[3];
        };

        struct Texture3D {
            VkDevice device;
            VkSampler sampler = VK_NULL_HANDLE;
            VkImage image = VK_NULL_HANDLE;
            VkImageLayout imageLayout;
            VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
            VkImageView view = VK_NULL_HANDLE;
            VkDescriptorImageInfo descriptor;
            VkFormat format;
            uint32_t width, height, depth;
            uint32_t mipLevels;

            void destroy() {
                if (view != VK_NULL_HANDLE)
                    vkDestroyImageView(device, view, nullptr);
                if (image != VK_NULL_HANDLE)
                    vkDestroyImage(device, image, nullptr);
                if (sampler != VK_NULL_HANDLE)
                    vkDestroySampler(device, sampler, nullptr);
                if (deviceMemory != VK_NULL_HANDLE)
                    vkFreeMemory(device, deviceMemory, nullptr);
            }
        };

        private:

            uint32_t m_msaaSamples = 64;

            VkPipeline m_pipeline;
            VkPipelineLayout m_pipelineLayout;

            Texture3D m_texture;

            struct {
                VkPipelineVertexInputStateCreateInfo inputState;
                std::vector<VkVertexInputBindingDescription> inputBinding;
                std::vector<VkVertexInputAttributeDescription> inputAttributes;
            } m_vertices;

            VulkanBuffer m_vertexBuffer;
            VulkanBuffer m_indexBuffer;
            uint32_t m_indexCount;

            VulkanBuffer m_uniformBufferVS;

            struct UBOVS {
                glm::mat4 projection;
                glm::mat4 modelView;
                glm::vec4 viewPos;
                float depth = 0.0f;
            } m_uboVS;

            VkDescriptorSet m_descriptorSet;
            VulkanDescriptorSetLayout m_descriptorSetLayout;

        public:
            VulkanExample() {
                srand((unsigned int)time(NULL));
            }
            ~VulkanExample() {
                m_descriptorSetLayout.cleanup();

                m_vertexBuffer.cleanup();
                m_indexBuffer.cleanup();
                m_texture.destroy();

                vkDestroyPipeline(m_device.getLogicalDevice(), m_pipeline, nullptr);

                vkDestroyPipelineLayout(m_device.getLogicalDevice(), m_pipelineLayout, nullptr);

                m_uniformBufferVS.cleanup();
            }

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

                prepareNoiseTexture(128, 128, 128);

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
                        &m_syncObjects.getInFlightFences()[m_currentFrame], 
                        VK_TRUE, 
                        UINT64_MAX);

                uint32_t imageIndex;

                VkResult result = vkAcquireNextImageKHR(
                        m_device.getLogicalDevice(), 
                        m_swapChain.getSwapChain(), 
                        UINT64_MAX, 
                        m_syncObjects.getImageAvailableSemaphores()[m_currentFrame], 
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
                    m_syncObjects.getInFlightFences()[m_currentFrame];

                updateUniformBuffers();

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
                submitInfo.pCommandBuffers = 
                    m_commandBuffers[imageIndex].getCommandBufferPointer();

                VkSemaphore signalSemaphores[] = {
                    m_syncObjects.getRenderFinishedSemaphores()[m_currentFrame]
                };

                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                vkResetFences(
                        m_device.getLogicalDevice(), 
                        1, 
                        &m_syncObjects.getInFlightFences()[m_currentFrame]);

                if (vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, m_syncObjects.getInFlightFences()[m_currentFrame]) != VK_SUCCESS) {
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

                m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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

            void  createDevice() override {
                m_device = VulkanDevice(
                        m_instance->getInstance(), 
                        m_surface.getSurface(), 
                        deviceExtensions,
                        enableValidationLayers, 
                        validationLayers, 
                        m_msaaSamples);
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

                VulkanShaderModule vertShaderModule= 
                    VulkanShaderModule(
                            "src/shaders/texture3d/texture3dVert.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_VERTEX_BIT);
                VulkanShaderModule fragShaderModule= 
                    VulkanShaderModule(
                            "src/shaders/texture3d/texture3dFrag.spv", 
                            &m_device, 
                            VK_SHADER_STAGE_FRAGMENT_BIT);

                VkPipelineShaderStageCreateInfo shaderStages[] = {
                    vertShaderModule.getStageCreateInfo(), 
                    fragShaderModule.getStageCreateInfo()
                };

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
                pipelineInfo.pVertexInputState = &m_vertices.inputState;
                
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

            void createUniformBuffers() {
                m_uniformBufferVS= VulkanBuffer(m_device);

                m_uniformBufferVS.createBuffer(
                        sizeof(m_uboVS), 
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                m_uniformBufferVS.map();

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

                    vkCmdBindDescriptorSets(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipelineLayout,

                            0, 
                            1, 
                            &m_descriptorSet,
                            0, 
                            nullptr);

                    vkCmdBindPipeline(
                            m_commandBuffers[i].getCommandBuffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            m_pipeline);

                    VkDeviceSize offsets[1] = { 0 };
                    vkCmdBindVertexBuffers(m_commandBuffers[i].getCommandBuffer(), 0, 1, m_vertexBuffer.getBufferPointer(), offsets);
                    vkCmdBindIndexBuffer(m_commandBuffers[i].getCommandBuffer(), m_indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(m_commandBuffers[i].getCommandBuffer(), m_indexCount, 1, 0, 0, 0);
                    
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
                descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
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
                poolSizes[0].descriptorCount = 1;
                poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[1].descriptorCount = 1;

                m_descriptorPool.create(poolSizes);
            }

            void createDescriptorSets() override {
                VkDescriptorSetAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool = m_descriptorPool.getDescriptorPool();
                allocInfo.descriptorSetCount = 1;
                allocInfo.pSetLayouts = m_descriptorSetLayout.getDescriptorSetLayoutPointer();

                VK_CHECK_RESULT(vkAllocateDescriptorSets(
                            m_device.getLogicalDevice(), 
                            &allocInfo, 
                            &m_descriptorSet));

                VkDescriptorImageInfo textureDescriptor = {};
                textureDescriptor.imageView = m_texture.view;
                textureDescriptor.imageLayout = m_texture.imageLayout;
                textureDescriptor.sampler = m_texture.sampler;

                std::vector<VkWriteDescriptorSet> descriptorWrites(2);

                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[0].descriptorCount = 1;
                descriptorWrites[0].pBufferInfo = m_uniformBufferVS.getDescriptorPointer();
                descriptorWrites[0].dstSet = m_descriptorSet;

                descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[1].dstBinding = 1;
                descriptorWrites[1].dstArrayElement = 0;
                descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[1].descriptorCount = 1;
                descriptorWrites[1].pImageInfo = &m_texture.descriptor;
                descriptorWrites[1].dstSet = m_descriptorSet;

                vkUpdateDescriptorSets(
                        m_device.getLogicalDevice(), 
                        descriptorWrites.size(), 
                        descriptorWrites.data(), 
                        0, NULL);
            }

            void updateUniformBuffers() {
                m_uboVS.projection = glm::perspective(
                        glm::radians(m_camera.getZoom()), 
                        m_swapChain.getExtent().width / (float) m_swapChain.getExtent().height, 
                        0.1f,  100.0f);
                m_uboVS.projection[1][1] *= -1;

                m_uboVS.modelView = m_camera.getViewMatrix();
                m_uboVS.viewPos = glm::vec4(m_camera.position(), 1.0f);

                m_uboVS.depth += m_fpsCounter.getDeltaTime() * 0.15f;
                if (m_uboVS.depth > 1.0f)
                    m_uboVS.depth = m_uboVS.depth - 1.0f;

                memcpy(m_uniformBufferVS.getMappedMemory(), &m_uboVS, sizeof(m_uboVS));
            }

            void prepareNoiseTexture(uint32_t width, uint32_t height, uint32_t depth) {
                m_texture.device = m_device.getLogicalDevice();
                m_texture.width = width;
                m_texture.height = height;
                m_texture.depth = depth;
                m_texture.mipLevels = 1;
                m_texture.format = VK_FORMAT_R8_UNORM;

                // Check if GPU supports 3D textures
                VkFormatProperties formatProperties;
                vkGetPhysicalDeviceFormatProperties(
                        m_device.getPhysicalDevice(), 
                        m_texture.format, 
                        &formatProperties);

                // Check if format supports transfer
                if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
                {
                    std::cout << "Error: Device does not support flag TRANSFER_DST for selected texture format!" << std::endl;
                    return;
                }
                // Check if GPU supports requested 3D texture dimensions
                uint32_t maxImageDimension3D(m_device.properties.limits.maxImageDimension3D);
                if (width > maxImageDimension3D 
                        || height > maxImageDimension3D 
                        || depth > maxImageDimension3D)
                {
                    std::cout << "Error: Requested texture dimensions is greater than supported 3D texture dimension!" << std::endl;
                    return;
                }

                VkImageCreateInfo imageCreateInfo = {};
                imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
                imageCreateInfo.format = m_texture.format;
                imageCreateInfo.mipLevels = m_texture.mipLevels;
                imageCreateInfo.arrayLayers = 1;
                imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                imageCreateInfo.extent.width = m_texture.width;
                imageCreateInfo.extent.height = m_texture.height;
                imageCreateInfo.extent.depth = m_texture.depth;
                imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                VK_CHECK_RESULT(vkCreateImage(
                            m_device.getLogicalDevice(), 
                            &imageCreateInfo, 
                            nullptr, 
                            &m_texture.image));

                VkMemoryAllocateInfo memAllocInfo = {};
                memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                VkMemoryRequirements memReqs = {};
                vkGetImageMemoryRequirements(
                        m_device.getLogicalDevice(), 
                        m_texture.image, 
                        &memReqs);

                memAllocInfo.allocationSize = memReqs.size;
                memAllocInfo.memoryTypeIndex = m_device.findMemoryType(
                        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                VK_CHECK_RESULT(vkAllocateMemory(
                            m_device.getLogicalDevice(), 
                            &memAllocInfo, 
                            nullptr, 
                            &m_texture.deviceMemory));

                VK_CHECK_RESULT(vkBindImageMemory(
                            m_device.getLogicalDevice(), 
                            m_texture.image, 
                            m_texture.deviceMemory, 0));

                VkSamplerCreateInfo sampler = {};
                sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                sampler.magFilter = VK_FILTER_LINEAR;
                sampler.minFilter = VK_FILTER_LINEAR;
                sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                sampler.mipLodBias = 0.0f;
                sampler.compareOp = VK_COMPARE_OP_NEVER;
                sampler.minLod = 0.0f;
                sampler.maxLod = 0.0f;
                sampler.maxAnisotropy = 1.0;
                sampler.anisotropyEnable = VK_FALSE;
                sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
                VK_CHECK_RESULT(vkCreateSampler(
                            m_device.getLogicalDevice(), 
                            &sampler, 
                            nullptr, 
                            &m_texture.sampler));

                VkImageViewCreateInfo view = {};
                view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                view.image = m_texture.image;
                view.viewType = VK_IMAGE_VIEW_TYPE_3D;
                view.format = m_texture.format;
                view.components = { 
                    VK_COMPONENT_SWIZZLE_R, 
                    VK_COMPONENT_SWIZZLE_G, 
                    VK_COMPONENT_SWIZZLE_B, 
                    VK_COMPONENT_SWIZZLE_A 
                };
                view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                view.subresourceRange.baseMipLevel = 0;
                view.subresourceRange.baseArrayLayer = 0;
                view.subresourceRange.layerCount = 1;
                view.subresourceRange.levelCount = 1;
                VK_CHECK_RESULT(vkCreateImageView(
                            m_device.getLogicalDevice(), 
                            &view, 
                            nullptr, 
                            &m_texture.view));

                m_texture.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                m_texture.descriptor.imageView = m_texture.view;
                m_texture.descriptor.sampler = m_texture.sampler;

                updateNoiseTexture();
            }

            void updateNoiseTexture() { 
                const uint32_t texMemSize = m_texture.width * m_texture.height * m_texture.depth;

                uint8_t *data = new uint8_t[texMemSize];
                memset(data, 0, texMemSize);

                auto tStart = std::chrono::high_resolution_clock::now();

                PerlinNoise<float> perlinNoise;
                FractalNoise<float> fractalNoise(perlinNoise);

                const float noiseScale = static_cast<float>(rand() % 10) + 4.0f;

// preprocessor directive to execute this for in parallel (multiple threads) 
#pragma omp parallel for
                for (int32_t z = 0; z < m_texture.depth; z++)
                {
                    for (int32_t y = 0; y < m_texture.height; y++)
                    {
                        for (int32_t x = 0; x < m_texture.width; x++)
                        {
                            float nx = (float)x / (float)m_texture.width;
                            float ny = (float)y / (float)m_texture.height;
                            float nz = (float)z / (float)m_texture.depth;
#define FRACTAL
#ifdef FRACTAL
                            float n = fractalNoise.noise(
                                    nx * noiseScale, 
                                    ny * noiseScale, 
                                    nz * noiseScale);
#else
                            float n = 20.0 * perlinNoise.noise(nx, ny, nz);
#endif
                            n = n - floor(n);

                            data[x + y * m_texture.width + z * m_texture.width * m_texture.height] 
                                = static_cast<uint8_t>(floor(n * 255));
                        }
                    }
                }

                VulkanBuffer stagingBuffer = VulkanBuffer(m_device);
                stagingBuffer.createBuffer(
                        texMemSize, 
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 
                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);


                // Copy texture data into staging buffer
                stagingBuffer.map(stagingBuffer.getMemoryRequirements()->size);
                memcpy(stagingBuffer.getMappedMemory(), data, texMemSize);
                stagingBuffer.unmap();

                VulkanCommandBuffer copyCmd;
                copyCmd.create(&m_device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

                VkImageSubresourceRange subresourceRange = {};
                subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                subresourceRange.baseMipLevel = 0;
                subresourceRange.levelCount = 1;
                subresourceRange.layerCount = 1;

                tools::setImageLayout(
                        copyCmd.getCommandBuffer(),
                        m_texture.image,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        subresourceRange);

                // Copy 3D noise data to texture
                VkBufferImageCopy bufferCopyRegion{};
                bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                bufferCopyRegion.imageSubresource.mipLevel = 0;
                bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
                bufferCopyRegion.imageSubresource.layerCount = 1;
                bufferCopyRegion.imageExtent.width = m_texture.width;
                bufferCopyRegion.imageExtent.height = m_texture.height;
                bufferCopyRegion.imageExtent.depth = m_texture.depth;

                vkCmdCopyBufferToImage(
                        copyCmd.getCommandBuffer(),
                        stagingBuffer.getBuffer(),
                        m_texture.image,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1,
                        &bufferCopyRegion);

                m_texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                tools::setImageLayout(
                        copyCmd.getCommandBuffer(),
                        m_texture.image,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        m_texture.imageLayout,
                        subresourceRange);

                copyCmd.flushCommandBuffer(&m_device, m_device.getGraphicsQueue(), true);

                // Clean up staging resources
                delete[] data;
                stagingBuffer.cleanup();
            }

            void loadAssets() {
                std::vector<Vertex> vertices =
                {
                    { {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
                    { { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
                    { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
                    { {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
                };

                std::vector<uint32_t> indices = { 0,1,2, 2,3,0 };
                m_indexCount = static_cast<uint32_t>(indices.size());

                m_vertexBuffer = VulkanBuffer(m_device);
                m_vertexBuffer.createBuffer(
                        vertices.size() * sizeof(Vertex), 
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        vertices.data());
                m_indexBuffer = VulkanBuffer(m_device);
                m_indexBuffer.createBuffer(
                        indices.size() * sizeof(uint32_t), 
                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        indices.data());

                m_vertices.inputBinding.resize(1);

                VkVertexInputBindingDescription vertexInputBinding = {};
                vertexInputBinding.binding = 0;
                vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                vertexInputBinding.stride = sizeof(Vertex);
                m_vertices.inputBinding[0] = vertexInputBinding;

                m_vertices.inputAttributes.resize(3);
                m_vertices.inputAttributes[0].binding   = 0;
                m_vertices.inputAttributes[0].format    = VK_FORMAT_R32G32B32_SFLOAT;
                m_vertices.inputAttributes[0].location  = 0;
                m_vertices.inputAttributes[0].offset    = offsetof(Vertex, pos);

                m_vertices.inputAttributes[1].binding   = 0;
                m_vertices.inputAttributes[1].format    = VK_FORMAT_R32G32_SFLOAT;
                m_vertices.inputAttributes[1].location  = 1;
                m_vertices.inputAttributes[1].offset    = offsetof(Vertex, uv);

                m_vertices.inputAttributes[2].binding   = 0;
                m_vertices.inputAttributes[2].format    = VK_FORMAT_R32G32B32_SFLOAT;
                m_vertices.inputAttributes[2].location  = 2;
                m_vertices.inputAttributes[2].offset    = offsetof(Vertex, normal);

                m_vertices.inputState = {};
                m_vertices.inputState.sType = 
                    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                m_vertices.inputState.vertexBindingDescriptionCount = 
                    m_vertices.inputBinding.size();
                m_vertices.inputState.pVertexBindingDescriptions = 
                    m_vertices.inputBinding.data();
                m_vertices.inputState.vertexAttributeDescriptionCount = 
                    m_vertices.inputAttributes.size();
                m_vertices.inputState.pVertexAttributeDescriptions = 
                    m_vertices.inputAttributes.data();

            }

            void OnUpdateUI (UI *ui) override {
                if (ui->header("Settings")) {
                    if (ui->button("Generate new texture")) {
                        updateNoiseTexture();
                    }
                }
            }
    };
}

VULKAN_EXAMPLE_MAIN()
