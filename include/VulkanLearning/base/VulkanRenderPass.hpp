#pragma once

#include <vulkan/vulkan.h>

#include "VulkanSwapChain.hpp"
#include "VulkanDevice.hpp"

namespace VulkanLearning {

    class VulkanRenderPass {
        private:
            VkRenderPass m_renderPass;

            VulkanSwapChain* m_swapChain;
            VulkanDevice* m_device;

        public:
            VulkanRenderPass(VulkanSwapChain* swapChain, VulkanDevice* device);
            ~VulkanRenderPass();

            VkRenderPass getRenderPass();

            void create();
            void create(
                    const std::vector<VkAttachmentDescription> attachments,
                    VkSubpassDescription subpass);
        private:
    };

}
