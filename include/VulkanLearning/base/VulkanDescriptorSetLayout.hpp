#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "VulkanDevice.hpp"

namespace VulkanLearning {

    class VulkanDescriptorSetLayout {
        private:
            VkDescriptorSetLayout m_descriptorSetLayout;

            VulkanDevice m_device;
        public:
            VulkanDescriptorSetLayout();
            VulkanDescriptorSetLayout(VulkanDevice device);

            ~VulkanDescriptorSetLayout();

            inline VkDescriptorSetLayout getDescriptorSetLayout() { return m_descriptorSetLayout; }
            inline VkDescriptorSetLayout* getDescriptorSetLayoutPointer() { return &m_descriptorSetLayout; }

            void create(std::vector<VkDescriptorSetLayoutBinding> bindings);
            void cleanup();
    };
}
