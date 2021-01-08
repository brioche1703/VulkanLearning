#pragma once

#include <vulkan/vulkan.h>

#include "VulkanDevice.hpp"

#include <memory>
#include <vector>
#include <string>
#include <fstream>

namespace VulkanLearning {
    class VulkanShaderModule {
        private:
            VkShaderModule m_module;
            VkPipelineShaderStageCreateInfo m_stageCreateInfo;

        public:
            VulkanShaderModule(std::string spvFileName, VulkanDevice* device, VkShaderStageFlagBits stage);
            ~VulkanShaderModule();

            inline VkShaderModule getModule() { return m_module; }
            inline VkPipelineShaderStageCreateInfo getStageCreateInfo() { return m_stageCreateInfo; }

        private:
            std::vector<char, std::allocator<char>> m_code;
            VkShaderModule createShaderModule(const std::vector<char>& code, VulkanDevice* device, VkShaderStageFlagBits stage);

            static std::vector<char> readFile(const std::string& filename) {
                std::ifstream file(filename, std::ios::ate | std::ios::binary);

                if (!file.is_open()) {
                    throw std::runtime_error(std::string {"File "} + 
                            filename + " opening failed!");
                }

                size_t fileSize = (size_t) file.tellg();
                std::vector<char> buffer(fileSize);

                file.seekg(0);
                file.read(buffer.data(), fileSize);

                file.close();

                return buffer;
            }

        public:
            void cleanup(VulkanDevice* device);
    };
}
