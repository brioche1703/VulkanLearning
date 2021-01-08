#include "UI.hpp"

namespace VulkanLearning {

    UI::UI() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGuiIO& io = ImGui::GetIO();
		io.FontGlobalScale = scale;
    }

    UI::~UI() {}

    void UI::create(VulkanDevice device, VulkanRenderPass renderPass) {
        m_device = device;
        m_queue = m_device.getGraphicsQueue();

        m_vertexBuffer = VulkanBuffer(m_device);
        m_indexBuffer = VulkanBuffer(m_device);

        VulkanShaderModule vertShaderModule = 
            VulkanShaderModule("src/shaders/uiVert.spv", &m_device, VK_SHADER_STAGE_VERTEX_BIT);
        VulkanShaderModule fragShaderModule = 
            VulkanShaderModule("src/shaders/uiFrag.spv", &m_device, VK_SHADER_STAGE_FRAGMENT_BIT);

        m_shaders.push_back(vertShaderModule);
        m_shaders.push_back(fragShaderModule);

        prepareResources();
        preparePipeline(nullptr, renderPass);
    }

    void UI::prepareResources() {
        ImGuiIO& io = ImGui::GetIO();

        // Create font texture
        unsigned char* fontData;
        int texWidth, texHeight;

        const std::string filename = "src/fonts/Roboto-Medium.ttf";
        io.Fonts->AddFontFromFileTTF(filename.c_str(), 14.0f);
        io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
        VkDeviceSize uploadSize = texWidth*texHeight * 4 * sizeof(char);

        VkImageCreateInfo imageInfo =  {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.extent.width = texWidth;
        imageInfo.extent.height = texHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VK_CHECK_RESULT(vkCreateImage(m_device.getLogicalDevice(), &imageInfo, nullptr, &m_fontImage));

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(m_device.getLogicalDevice(), m_fontImage, &memReqs);
        VkMemoryAllocateInfo memAllocInfo = {};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex = m_device.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK_RESULT(vkAllocateMemory(m_device.getLogicalDevice(), &memAllocInfo, nullptr, &m_fontMemory));
        VK_CHECK_RESULT(vkBindImageMemory(m_device.getLogicalDevice(), m_fontImage, m_fontMemory, 0));

        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_fontImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        VK_CHECK_RESULT(vkCreateImageView(m_device.getLogicalDevice(), &viewInfo, nullptr, &m_fontView));

        VulkanBuffer stagingBuffer = VulkanBuffer(m_device);
        stagingBuffer.createBuffer(
                uploadSize, 
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        stagingBuffer.map();
        memcpy(stagingBuffer.getMappedMemory(), fontData, uploadSize);
        stagingBuffer.unmap();

        VulkanCommandBuffer copyCmd = VulkanCommandBuffer();
        copyCmd.create(&m_device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkImageSubresourceRange subresourceRangeCopy = {};
        subresourceRangeCopy.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRangeCopy.baseMipLevel = 0;
        subresourceRangeCopy.levelCount = 1;
        subresourceRangeCopy.layerCount = 1;

        VkImageMemoryBarrier imageMemoryBarrierCopy = {};
        imageMemoryBarrierCopy.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrierCopy.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrierCopy.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrierCopy.image = m_fontImage;
        imageMemoryBarrierCopy.subresourceRange = subresourceRangeCopy;
        imageMemoryBarrierCopy.srcAccessMask = 0;
        imageMemoryBarrierCopy.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

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
                &imageMemoryBarrierCopy);

        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = texWidth;
        bufferCopyRegion.imageExtent.height = texHeight;
        bufferCopyRegion.imageExtent.depth = 1;

        vkCmdCopyBufferToImage(
                copyCmd.getCommandBuffer(), 
                stagingBuffer.getBuffer(),
                m_fontImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &bufferCopyRegion);

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;

        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageMemoryBarrier.image = m_fontImage;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

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

        copyCmd.flushCommandBuffer(&m_device, m_queue, true);

        stagingBuffer.cleanup();

        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VK_CHECK_RESULT(vkCreateSampler(m_device.getLogicalDevice(), &samplerInfo, nullptr, &m_sampler));

        // Descriptor Pool
        std::vector<VkDescriptorPoolSize> poolSizes = std::vector<VkDescriptorPoolSize>(1);
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = 1;
        m_descriptorPool = VulkanDescriptorPool(m_device);
        m_descriptorPool.create(poolSizes, 2);

        // Descriptor Set Layout
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = 
            std::vector<VkDescriptorSetLayoutBinding>(1);
        setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        setLayoutBindings[0].binding = 0;
        setLayoutBindings[0].descriptorCount = 1;
        m_descriptorSetLayout = VulkanDescriptorSetLayout(m_device);
        m_descriptorSetLayout.create(setLayoutBindings);

        // Descriptor Set
        VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {};
        descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocInfo.descriptorPool = m_descriptorPool.getDescriptorPool();
        descriptorSetAllocInfo.descriptorSetCount = 1;
        descriptorSetAllocInfo.pSetLayouts = m_descriptorSetLayout.getDescriptorSetLayoutPointer();
        VK_CHECK_RESULT(vkAllocateDescriptorSets(
                    m_device.getLogicalDevice(), 
                    &descriptorSetAllocInfo, 
                    &m_descriptorSet));

        VkDescriptorImageInfo fontDescriptor = {};
        fontDescriptor.sampler = m_sampler;
        fontDescriptor.imageView = m_fontView;
        fontDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        std::vector<VkWriteDescriptorSet> descriptorWrites = 
            std::vector<VkWriteDescriptorSet>(1);
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_descriptorSet;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &fontDescriptor;

        vkUpdateDescriptorSets(
                m_device.getLogicalDevice(), 
                static_cast<uint32_t>(descriptorWrites.size()), 
                descriptorWrites.data(),
                0,
                nullptr);
    }

    void UI::preparePipeline(const VkPipelineCache pipelineCache, const VulkanRenderPass renderPass)
    {
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.size = sizeof(PushConstBlock);
        pushConstantRange.offset = 0;

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.pSetLayouts = m_descriptorSetLayout.getDescriptorSetLayoutPointer();
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
        VK_CHECK_RESULT(vkCreatePipelineLayout(
                    m_device.getLogicalDevice(),
                    &pipelineLayoutCreateInfo, 
                    nullptr, 
                    &m_pipelineLayout));

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
        inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyState.flags = 0;
        inputAssemblyState.primitiveRestartEnable = VK_FALSE;

        VkPipelineRasterizationStateCreateInfo rasterizationState = {};
        rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationState.cullMode = VK_CULL_MODE_NONE;
        rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationState.flags = 0;
        rasterizationState.depthClampEnable = VK_FALSE;
        rasterizationState.lineWidth = 1.0f;

        // Enable blending
        VkPipelineColorBlendAttachmentState blendAttachmentState = {};
        blendAttachmentState.blendEnable = VK_TRUE;
        blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlendState = {};
        colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendState.attachmentCount = 1;
        colorBlendState.pAttachments = &blendAttachmentState;

        VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
        depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilState.depthTestEnable = VK_FALSE;
        depthStencilState.depthWriteEnable = VK_FALSE;
        depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        viewportState.flags = 0;

        VkPipelineMultisampleStateCreateInfo multisampleState = {};
        multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleState.rasterizationSamples = m_device.getMsaaSamples();
        multisampleState.flags = 0;

        std::vector<VkDynamicState> dynamicStateEnables = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.pDynamicStates = dynamicStateEnables.data();
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
        dynamicState.flags = 0;

        VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.layout = m_pipelineLayout;
        pipelineCreateInfo.renderPass = renderPass.getRenderPass();
        pipelineCreateInfo.flags = 0;
        pipelineCreateInfo.basePipelineIndex = -1;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineCreateInfo.pRasterizationState = &rasterizationState;
        pipelineCreateInfo.pColorBlendState = &colorBlendState;
        pipelineCreateInfo.pMultisampleState = &multisampleState;
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pDepthStencilState = &depthStencilState;
        pipelineCreateInfo.pDynamicState = &dynamicState;
        pipelineCreateInfo.stageCount = static_cast<uint32_t>(m_shaders.size());
        std::vector<VkPipelineShaderStageCreateInfo> shadersData;
        std::transform(
                m_shaders.begin(), m_shaders.end(), 
                std::back_inserter(shadersData), 
                [](VulkanShaderModule s) -> VkPipelineShaderStageCreateInfo {return s.getStageCreateInfo();});
        pipelineCreateInfo.pStages = shadersData.data();
        pipelineCreateInfo.subpass = m_subpass;

        std::vector<VkVertexInputBindingDescription> vertexInputBindings = 
            std::vector<VkVertexInputBindingDescription>(1);
        vertexInputBindings[0].binding = 0;
        vertexInputBindings[0].stride = sizeof(ImDrawVert);
        vertexInputBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = 
            std::vector<VkVertexInputAttributeDescription>(3);

        vertexInputAttributes[0].binding = 0;
        vertexInputAttributes[0].location = 0;
        vertexInputAttributes[0].format = VK_FORMAT_R32G32_SFLOAT;
        vertexInputAttributes[0].offset = offsetof(ImDrawVert, pos);

        vertexInputAttributes[1].binding = 0;
        vertexInputAttributes[1].location = 1;
        vertexInputAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
        vertexInputAttributes[1].offset = offsetof(ImDrawVert, uv);

        vertexInputAttributes[2].binding = 0;
        vertexInputAttributes[2].location = 2;
        vertexInputAttributes[2].format = VK_FORMAT_R8G8B8A8_UNORM;
        vertexInputAttributes[2].offset = offsetof(ImDrawVert, col);

        VkPipelineVertexInputStateCreateInfo vertexInputState = {};
        vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
        vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
        vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
        vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

        pipelineCreateInfo.pVertexInputState = &vertexInputState;

        VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_device.getLogicalDevice(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &m_pipeline));
    }

    bool UI::update()
    {
        ImDrawData* imDrawData = ImGui::GetDrawData();
        bool updateCmdBuffers = false;

        if (!imDrawData) { return false; };

        // Note: Alignment is done inside buffer creation
        VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
        VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

        // Update buffers only if vertex or index count has been changed compared to current buffer size
        if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
             return false;
        }

        // Vertex buffer
        if ((m_vertexBuffer.getBuffer() == VK_NULL_HANDLE) || (m_vertexCount != imDrawData->TotalVtxCount)) {
            m_vertexBuffer.unmap();
            m_vertexBuffer.cleanup();
            m_vertexBuffer.createBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            m_vertexCount = imDrawData->TotalVtxCount;
            m_vertexBuffer.map();
            updateCmdBuffers = true;
        }

        // Index buffer
        VkDeviceSize indexSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
        if ((m_indexBuffer.getBuffer() == VK_NULL_HANDLE) || (m_indexCount < imDrawData->TotalIdxCount)) {
            m_indexBuffer.unmap();
            m_indexBuffer.cleanup();
            m_indexBuffer.createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            m_indexCount = imDrawData->TotalIdxCount;
            m_indexBuffer.map();
            updateCmdBuffers = true;
        }

        // Upload data
        ImDrawVert* vtxDst = (ImDrawVert*)m_vertexBuffer.getMappedMemory();
        ImDrawIdx* idxDst = (ImDrawIdx*)m_indexBuffer.getMappedMemory();

        for (int n = 0; n < imDrawData->CmdListsCount; n++) {
            const ImDrawList* cmd_list = imDrawData->CmdLists[n];
            memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtxDst += cmd_list->VtxBuffer.Size;
            idxDst += cmd_list->IdxBuffer.Size;
        }

        // Flush to make writes visible to GPU
        m_vertexBuffer.flush();
        m_indexBuffer.flush();

        return updateCmdBuffers;
    }

    void UI::draw(const VkCommandBuffer commandBuffer)
    {
        ImDrawData* imDrawData = ImGui::GetDrawData();
        int32_t vertexOffset = 0;
        int32_t indexOffset = 0;

        if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, NULL);

        pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
        pushConstBlock.translate = glm::vec2(-1.0f);
        vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, m_vertexBuffer.getBufferPointer(), offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT16);

        for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
        {
            const ImDrawList* cmd_list = imDrawData->CmdLists[i];
            for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
                VkRect2D scissorRect;
                scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
                scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
                scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
                vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
                vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
                indexOffset += pcmd->ElemCount;
            }
            vertexOffset += cmd_list->VtxBuffer.Size;
        }
    }

    void UI::resize(uint32_t width, uint32_t height)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)width, (float)height);
    }

    void UI::freeResources()
    {
        ImGui::DestroyContext();
        m_vertexBuffer.cleanup();
        m_indexBuffer.cleanup();
        for (VulkanShaderModule shader : m_shaders) {
            shader.cleanup(&m_device);
        }
        vkDestroyImageView(m_device.getLogicalDevice(), m_fontView, nullptr);
        vkDestroyImage(m_device.getLogicalDevice(), m_fontImage, nullptr);
        vkFreeMemory(m_device.getLogicalDevice(), m_fontMemory, nullptr);
        vkDestroySampler(m_device.getLogicalDevice(), m_sampler, nullptr);
        vkDestroyDescriptorSetLayout(m_device.getLogicalDevice(), m_descriptorSetLayout.getDescriptorSetLayout(), nullptr);
        vkDestroyDescriptorPool(m_device.getLogicalDevice(), m_descriptorPool.getDescriptorPool(), nullptr);
        vkDestroyPipelineLayout(m_device.getLogicalDevice(), m_pipelineLayout, nullptr);
        vkDestroyPipeline(m_device.getLogicalDevice(), m_pipeline, nullptr);
    }

    bool UI::header(const char *caption)
    {
        return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
    }

    bool UI::checkBox(const char *caption, bool *value)
    {
        bool res = ImGui::Checkbox(caption, value);
        if (res) { updated = true; };
        return res;
    }

    bool UI::checkBox(const char *caption, int32_t *value)
    {
        bool val = (*value == 1);
        bool res = ImGui::Checkbox(caption, &val);
        *value = val;
        if (res) { updated = true; };
        return res;
    }

    bool UI::inputFloat(const char *caption, float *value, float step, uint32_t precision)
    {
        bool res = ImGui::InputFloat(caption, value, step, step * 10.0f, precision);
        if (res) { updated = true; };
        return res;
    }

    bool UI::sliderFloat(const char* caption, float* value, float min, float max)
    {
        bool res = ImGui::SliderFloat(caption, value, min, max);
        if (res) { updated = true; };
        return res;
    }

    bool UI::sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max)
    {
        bool res = ImGui::SliderInt(caption, value, min, max);
        if (res) { updated = true; };
        return res;
    }

    bool UI::comboBox(const char *caption, int32_t *itemindex, std::vector<std::string> items)
    {
        if (items.empty()) {
            return false;
        }
        std::vector<const char*> charitems;
        charitems.reserve(items.size());
        for (size_t i = 0; i < items.size(); i++) {
            charitems.push_back(items[i].c_str());
        }
        uint32_t itemCount = static_cast<uint32_t>(charitems.size());
        bool res = ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
        if (res) { updated = true; };
        return res;
    }

    bool UI::button(const char *caption)
    {
        bool res = ImGui::Button(caption);
        if (res) { updated = true; };
        return res;
    }

    void UI::text(const char *formatstr, ...)
    {
        va_list args;
        va_start(args, formatstr);
        ImGui::TextV(formatstr, args);
        va_end(args);
    }

}
