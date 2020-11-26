#pragma once

#include <string>

#include "ktx.h"

#include "VulkanDevice.hpp"

namespace VulkanLearning {

    class VulkanTextureNew {
        private:
            VulkanDevice m_device;
            VkImage m_image;
            VkImageLayout m_imageLayout;
            VkDeviceMemory m_deviceMemory;
            VkImageView m_view;
            uint32_t m_width;
            uint32_t m_height;
            uint32_t m_mipLevels;
            uint32_t m_layourCount;
            VkDescriptorImageInfo m_descriptor;
            VkSampler m_sampler;

        public:
            VulkanTextureNew();
            ~VulkanTextureNew();


            ktxResult loadKTXFile(std::string filename, ktxTexture **target);
            void destroy();
    };

}
