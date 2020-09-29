#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"

namespace VulkanLearning {
    class VulkanSyncObjects {
        private:
            std::vector<VkSemaphore> m_imageAvailableSemaphores;
            std::vector<VkSemaphore> m_renderFinishedSemaphores;
            std::vector<VkFence> m_inFlightFences;
            std::vector<VkFence> m_imagesInFlight;

            VulkanDevice* m_device;
            VulkanSwapChain* m_swapChain;

            const int m_maxFrameInFlight = 2;

        public:
            VulkanSyncObjects(VulkanDevice* device, VulkanSwapChain* swapChain,
                    int maxFrameInFlight);
            ~VulkanSyncObjects();

            std::vector<VkSemaphore> getImageAvailableSemaphores() { return m_imageAvailableSemaphores; }
            std::vector<VkSemaphore> getRenderFinishedSemaphores() { return m_renderFinishedSemaphores; }
            std::vector<VkFence> getInFlightFences() { return m_inFlightFences; }
            std::vector<VkFence> getImagesInFlight() { return m_imagesInFlight; }

            void create();
    };
}

