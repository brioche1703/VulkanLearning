#include "../../include/VulkanLearning/base/VulkanRenderPass.hpp"

namespace VulkanLearning {

    VulkanRenderPass::VulkanRenderPass(VulkanSwapChain* swapChain, VulkanDevice* device) 
    : m_swapChain(swapChain), m_device(device) {
    }

    VulkanRenderPass::~VulkanRenderPass() {}

    VkRenderPass VulkanRenderPass::getRenderPass() { return m_renderPass; }

    void VulkanRenderPass::create(
            const std::vector<VkAttachmentDescription> attachments,
            VkSubpassDescription subpass) {
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(m_device->getLogicalDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Render pass creation failed!");
        }
    }

}
