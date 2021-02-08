#pragma once

#include <vulkan/vulkan.h>

#include "VulkanSwapChain.hpp"
#include "VulkanDevice.hpp"

namespace VulkanLearning {

    class VulkanRenderPass {
        private:
            VkRenderPass m_renderPass;

            VulkanSwapChain m_swapChain;
            VulkanDevice m_device;

        public:
            VulkanRenderPass();
            VulkanRenderPass(VulkanSwapChain swapChain, VulkanDevice device);
            ~VulkanRenderPass();

            VkRenderPass getRenderPass() const;

            void create(const std::vector<VkAttachmentDescription> attachment, VkSubpassDescription subpass);
            void create(const std::vector<VkSubpassDependency> dependency, const std::vector<VkAttachmentDescription> attachment, const std::vector<VkSubpassDescription> subpass);

        private:
    };

}
