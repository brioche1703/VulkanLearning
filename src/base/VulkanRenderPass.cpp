#include "VulkanRenderPass.hpp"

namespace VulkanLearning {

    VulkanRenderPass::VulkanRenderPass() {}

    VulkanRenderPass::VulkanRenderPass(VulkanSwapChain swapChain, VulkanDevice device) 
    : m_swapChain(swapChain), m_device(device) {
    }

    VulkanRenderPass::~VulkanRenderPass() {}

    VkRenderPass VulkanRenderPass::getRenderPass() const { return m_renderPass; }

    void VulkanRenderPass::create(
            const std::vector<VkAttachmentDescription> attachment,
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
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachment.size());
        renderPassInfo.pAttachments = attachment.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(m_device.getLogicalDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Render pass creation failed!");
        }
    }

    void VulkanRenderPass::create(
            const std::vector<VkSubpassDependency> dependency,
            const std::vector<VkAttachmentDescription> attachment,
            const std::vector<VkSubpassDescription> subpass) {
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachment.size());
        renderPassInfo.pAttachments = attachment.data();
        renderPassInfo.subpassCount = static_cast<uint32_t>(subpass.size());
        renderPassInfo.pSubpasses = subpass.data();
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependency.size());
        renderPassInfo.pDependencies = dependency.data();

        if (vkCreateRenderPass(m_device.getLogicalDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Render pass creation failed!");
        }
    }

}
