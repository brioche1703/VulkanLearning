#include "VulkanBase.hpp"

#include "ktx.h"
#include <cstring>
#include <vulkan/vulkan_core.h>

namespace VulkanLearning {

    const std::string TEXTURE_PATH = "./src/textures/metalplate01_rgba.ktx";

    struct Vertex {
        glm::vec3 pos;
        glm::vec2 uv;
        glm::vec3 normal;
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

                destroyTextureImage(texture);

                m_descriptorSetLayout.cleanup();

                m_vertexBuffer.cleanup();
                m_indexBuffer.cleanup();
            }

            void run() {
                VulkanBase::run();
            }

            struct Texture {
                VkSampler sampler;
                VkImage image;
                VkImageLayout imageLayout;
                VkDeviceMemory deviceMemory;
                VkImageView view;
                uint32_t width, height;
                uint32_t mipLevels;
            } texture;

        private:
            std::vector<Vertex> m_vertices =
            {
                { {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
                { { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
                { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
                { {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
            };

            std::vector<uint32_t> m_indices = { 0,1,2, 2,3,0 };

            void initVulkan() override {
                m_msaaSamples = 64;
                VulkanBase::initVulkan();

                m_window.setTitle("Texture");
                m_camera.setPosition(glm::vec3(0.0f, 0.0f, 3.0f));

                createTextureKTX();
                createVertexBuffer();
                createIndexBuffer();

                createDescriptorSetLayout();
                createPipelines();

                createCoordinateSystemUniformBuffers();
                createDescriptorPool();
                createDescriptorSets();
                createCommandBuffers();
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
                    VulkanShaderModule("src/shaders/textureVert.spv", &m_device, VK_SHADER_STAGE_VERTEX_BIT);
                VulkanShaderModule fragShaderModule = 
                    VulkanShaderModule("src/shaders/textureFrag.spv", &m_device, VK_SHADER_STAGE_FRAGMENT_BIT);

                VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
                vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

                VkVertexInputBindingDescription vertexBindingDescription;
                vertexBindingDescription.binding = 0;
                vertexBindingDescription.stride = sizeof(Vertex);
                vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                std::vector<VkVertexInputAttributeDescription> vertexAttributeDescription;
                vertexAttributeDescription.resize(3);
                vertexAttributeDescription[0].binding = 0;
                vertexAttributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
                vertexAttributeDescription[0].location = 0;
                vertexAttributeDescription[0].offset = offsetof(Vertex, pos);

                vertexAttributeDescription[1].binding = 0;
                vertexAttributeDescription[1].format = VK_FORMAT_R32G32_SFLOAT;
                vertexAttributeDescription[1].location = 1;
                vertexAttributeDescription[1].offset = offsetof(Vertex, uv);

                vertexAttributeDescription[2].binding = 0;
                vertexAttributeDescription[2].format = VK_FORMAT_R32G32B32_SFLOAT;
                vertexAttributeDescription[2].location = 2;
                vertexAttributeDescription[2].offset = offsetof(Vertex, normal);

                vertexInputInfo.vertexBindingDescriptionCount = 1;
                vertexInputInfo.vertexAttributeDescriptionCount = 
                    static_cast<uint32_t>(vertexAttributeDescription.size());
                vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
                vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();

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

            void destroyTextureImage(Texture texture) {
                vkDestroyImageView(m_device.getLogicalDevice(), texture.view, nullptr);
                vkDestroyImage(m_device.getLogicalDevice(), texture.image, nullptr);
                vkDestroySampler(m_device.getLogicalDevice(), texture.sampler, nullptr);
                vkFreeMemory(m_device.getLogicalDevice(), texture.deviceMemory, nullptr);
            }

            void createTextureKTX() {
                VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

                ktxResult result;
                ktxTexture* ktxTexture;

                result = ktxTexture_CreateFromNamedFile(TEXTURE_PATH.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

                if (result != KTX_SUCCESS) {
                    throw std::runtime_error("KTX Texture creation failed!");
                }

                texture.width = ktxTexture->baseWidth;
                texture.height = ktxTexture->baseHeight;
                texture.mipLevels = ktxTexture->numLevels;
              
                ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);
                ktx_size_t ktxTextureSize = ktxTexture_GetDataSize(ktxTexture);

                bool forceLinearTiling = true;
                if (forceLinearTiling) {
                    VkFormatProperties formatProperties;
                    vkGetPhysicalDeviceFormatProperties(m_device.getPhysicalDevice(), format, &formatProperties);
                }

                VkMemoryAllocateInfo memAllocInfo = {};
                memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

                VkMemoryRequirements memReqs = {};

                    VkBuffer stagingBuffer;
                    VkDeviceMemory stagingMemory;

                    VkBufferCreateInfo bufferCreateInfo = {};
                    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferCreateInfo.size = ktxTextureSize;
                    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                    if (vkCreateBuffer(m_device.getLogicalDevice(), &bufferCreateInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
                        throw std::runtime_error("Buffer creation failed!");
                    }

                    vkGetBufferMemoryRequirements(m_device.getLogicalDevice(), stagingBuffer, &memReqs);
                    memAllocInfo.allocationSize = memReqs.size;
                    memAllocInfo.memoryTypeIndex = m_device.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                    if (vkAllocateMemory(m_device.getLogicalDevice(), &memAllocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
                        throw std::runtime_error("Memory allocation failed!");
                    }

                    if (vkBindBufferMemory(m_device.getLogicalDevice(), stagingBuffer, stagingMemory, 0) != VK_SUCCESS) {
                        throw std::runtime_error("Buffer memory binding failed!");
                    }

                    uint8_t * data;
                    if (vkMapMemory(m_device.getLogicalDevice(), stagingMemory, 0, memReqs.size, 0, (void **)&data) != VK_SUCCESS) {
                        throw std::runtime_error("Mapping memory failed!");
                    }
                    memcpy(data, ktxTextureData, ktxTextureSize);
                    vkUnmapMemory(m_device.getLogicalDevice(), stagingMemory);

                    std::vector<VkBufferImageCopy> bufferCopyRegions;

                    uint32_t offset = 0;

                    for (uint32_t i = 0; i < texture.mipLevels; i++) {
                        ktx_size_t offset;
                        if (ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset) != KTX_SUCCESS) {
                            throw std::runtime_error("KTX get image offset failed!");
                        }

                        VkBufferImageCopy bufferCopyRegion = {};
                        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        bufferCopyRegion.imageSubresource.mipLevel = i;
                        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
                        bufferCopyRegion.imageSubresource.layerCount = 1;
                        bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> i;
                        bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> i;
                        bufferCopyRegion.imageExtent.depth = 1;
                        bufferCopyRegion.bufferOffset = offset;
                        bufferCopyRegions.push_back(bufferCopyRegion);
                    }

                    VkImageCreateInfo imageCreateInfo = {};
                    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
                    imageCreateInfo.format = format;
                    imageCreateInfo.mipLevels = texture.mipLevels;
                    imageCreateInfo.arrayLayers = 1;
                    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    imageCreateInfo.extent = { texture.width, texture.height, 1 };
                    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

                    if (vkCreateImage(m_device.getLogicalDevice(), &imageCreateInfo, nullptr, &texture.image) != VK_SUCCESS) {
                        throw std::runtime_error("Image creation failed!");
                    }

                    vkGetImageMemoryRequirements(m_device.getLogicalDevice(), texture.image, &memReqs);
                    memAllocInfo.allocationSize = memReqs.size;
                    memAllocInfo.memoryTypeIndex = m_device.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                    if (vkAllocateMemory(m_device.getLogicalDevice(), &memAllocInfo, nullptr, &texture.deviceMemory) != VK_SUCCESS) {
                        throw std::runtime_error("Memory allocation failed!");
                    }

                    if (vkBindImageMemory(m_device.getLogicalDevice(), texture.image, texture.deviceMemory, 0) != VK_SUCCESS) {
                        throw std::runtime_error("Image memory binding failed!");
                    }

                    VulkanCommandBuffer copyCmd;
                    copyCmd.create(&m_device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

                    VkImageSubresourceRange subresourceRange = {};
                    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    subresourceRange.baseMipLevel = 0;
                    subresourceRange.levelCount = texture.mipLevels;
                    subresourceRange.layerCount = 1;

                    VkImageMemoryBarrier imageMemoryBarrier = {};
                    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    imageMemoryBarrier.image = texture.image;
                    imageMemoryBarrier.subresourceRange = subresourceRange;
                    imageMemoryBarrier.srcAccessMask = 0;
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

                    vkCmdPipelineBarrier(
                            copyCmd.getCommandBuffer(), 
                            VK_PIPELINE_STAGE_HOST_BIT, 
                            VK_PIPELINE_STAGE_TRANSFER_BIT, 
                            0,
                            0,
                            nullptr,
                            0,
                            nullptr,
                            1,
                            &imageMemoryBarrier);
                           
                    vkCmdCopyBufferToImage(
                            copyCmd.getCommandBuffer(), 
                            stagingBuffer, 
                            texture.image, 
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                            static_cast<uint32_t>(bufferCopyRegions.size()), 
                            bufferCopyRegions.data());

                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    vkCmdPipelineBarrier(
                            copyCmd.getCommandBuffer(), 
                            VK_PIPELINE_STAGE_TRANSFER_BIT, 
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
                            0,
                            0,
                            nullptr,
                            0,
                            nullptr,
                            1,
                            &imageMemoryBarrier);

                    texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    copyCmd.flushCommandBuffer(&m_device, true);

                    vkFreeMemory(m_device.getLogicalDevice(), stagingMemory, nullptr);
                    vkDestroyBuffer(m_device.getLogicalDevice(), stagingBuffer, nullptr);

                ktxTexture_Destroy(ktxTexture);

                VkSamplerCreateInfo sampler = {};
                sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                sampler.magFilter = VK_FILTER_LINEAR;
                sampler.minFilter = VK_FILTER_LINEAR;
                sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler.mipLodBias = 0.0f;
                sampler.compareOp = VK_COMPARE_OP_NEVER;
                sampler.minLod = 0.0f;
                sampler.maxLod = (float)texture.mipLevels;
                if (m_device.features.samplerAnisotropy) {
                    sampler.anisotropyEnable = VK_TRUE;
                    sampler.maxAnisotropy = 16.0f;
                } else {
                    sampler.anisotropyEnable = VK_FALSE;
                    sampler.maxAnisotropy = 1.0f;
                }

                sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

                if (vkCreateSampler(m_device.getLogicalDevice(), &sampler, 
                            nullptr, &texture.sampler) != VK_SUCCESS) {
                    throw std::runtime_error("Sampler creation failed!");
                }

                VkImageViewCreateInfo view = {};
                view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                view.viewType = VK_IMAGE_VIEW_TYPE_2D;
                view.format = format;
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
                view.subresourceRange.levelCount = texture.mipLevels;
                view.image = texture.image;

                if (vkCreateImageView(m_device.getLogicalDevice(), &view, 
                            nullptr, &texture.view) != VK_SUCCESS) {
                    throw std::runtime_error("Image view creation failed!");
                }
            }
 
            void createVertexBuffer() override {
                m_vertexBuffer = VulkanBuffer(m_device);
                m_vertexBuffer.createWithStagingBuffer(m_vertices,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            }

            void createIndexBuffer() override {
                m_indexBuffer = VulkanBuffer(m_device);
                m_indexBuffer.createWithStagingBuffer(m_indices,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            }

            void createCoordinateSystemUniformBuffers() override {
                VkDeviceSize bufferSize = sizeof(CoordinatesSystemUniformBufferObject);

                m_coordinateSystemUniformBuffers.resize(m_swapChain.getImages().size());

                for (size_t i = 0; i < m_swapChain.getImages().size(); i++) {
                    m_coordinateSystemUniformBuffers[i] = VulkanBuffer(m_device);
                    m_coordinateSystemUniformBuffers[i].createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, *m_coordinateSystemUniformBuffers[i].getBufferPointer(), *m_coordinateSystemUniformBuffers[i].getBufferMemoryPointer());
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
                clearValues[1].depthStencil = {1.0f, 0};

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
                            static_cast<uint32_t>(m_indices.size()), 
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

            void createDescriptorSetLayout() override {
                m_descriptorSetLayout = VulkanDescriptorSetLayout(m_device);

                VkDescriptorSetLayoutBinding uboLayoutBinding{};
                uboLayoutBinding.binding = 0;
                uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uboLayoutBinding.descriptorCount = 1;
                uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                uboLayoutBinding.pImmutableSamplers = nullptr;

                VkDescriptorSetLayoutBinding samplerLayoutBinding{};
                samplerLayoutBinding.binding = 1;
                samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                samplerLayoutBinding.descriptorCount = 1;
                samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                samplerLayoutBinding.pImmutableSamplers = nullptr;
        
                std::vector<VkDescriptorSetLayoutBinding> bindings = 
                {uboLayoutBinding, samplerLayoutBinding};

                m_descriptorSetLayout.create(bindings);
            }

            void createDescriptorPool() override {
                m_descriptorPool = VulkanDescriptorPool(m_device, m_swapChain);

                std::vector<VkDescriptorPoolSize> poolSizes = std::vector<VkDescriptorPoolSize>(2);
                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = static_cast<uint32_t>(
                        m_swapChain.getImages().size());
                poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[1].descriptorCount = static_cast<uint32_t>(
                        m_swapChain.getImages().size());

                m_descriptorPool.create(poolSizes);
            }

            void createDescriptorSets() override {
                std::vector<std::vector<VulkanBuffer>> ubos{
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
                        std::vector<VkWriteDescriptorSet>(2);

                    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrites[0].dstBinding = 0;
                    descriptorWrites[0].dstArrayElement = 0;
                    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    descriptorWrites[0].descriptorCount = 1;
                    descriptorWrites[0].pBufferInfo = &bufferInfo;

                    VkDescriptorImageInfo imageInfo{};
                    imageInfo.imageView = texture.view;
                    imageInfo.sampler = texture.sampler;
                    imageInfo.imageLayout = texture.imageLayout;

                    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrites[1].dstBinding = 1;
                    descriptorWrites[1].dstArrayElement = 0;
                    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    descriptorWrites[1].descriptorCount = 1;
                    descriptorWrites[1].pImageInfo = &imageInfo;

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
