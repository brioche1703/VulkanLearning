#pragma once

#include <vulkan/vulkan.h>

#include "VulkanSwapChain.hpp"
#include "VulkanDevice.hpp"

namespace VulkanLearning {

    class VulkanRenderPass {
        private:
            VkRenderPass m_renderPass;

        public:
            VulkanRenderPass(VulkanSwapChain* swapChain, VulkanDevice* device);
            ~VulkanRenderPass();

            VkRenderPass getRenderPass();

        private:
            void create(VulkanSwapChain* swapChain, VulkanDevice* device);
    };

}
