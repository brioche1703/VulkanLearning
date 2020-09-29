#include "../../include/VulkanLearning/base/VulkanSyncObjects.hpp"

namespace VulkanLearning {

    VulkanSyncObjects::VulkanSyncObjects(VulkanDevice* device,
            VulkanSwapChain* swapChain, int maxFrameInFlight = 2) : 
            m_device(device), m_swapChain(swapChain) , 
            m_maxFrameInFlight(maxFrameInFlight){
        create();
    }

    VulkanSyncObjects::~VulkanSyncObjects() {}

    void VulkanSyncObjects::create() {
        m_imageAvailableSemaphores.resize(m_maxFrameInFlight);
        m_renderFinishedSemaphores.resize(m_maxFrameInFlight);
        m_inFlightFences.resize(m_maxFrameInFlight);
        m_imagesInFlight.resize(m_swapChain->getImages().size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;


        for (size_t i = 0; i < m_maxFrameInFlight; i++) {
            if (vkCreateSemaphore(m_device->getLogicalDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS 
                    || vkCreateSemaphore(m_device->getLogicalDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS
                    || vkCreateFence(m_device->getLogicalDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("Synchronisation objects creation for a frame failed!");
            }
        }
    }
}
