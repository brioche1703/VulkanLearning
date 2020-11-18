#include "VulkanShaderModule.hpp"

namespace VulkanLearning {
    VulkanShaderModule::VulkanShaderModule(std::string spvFileName, 
            VulkanDevice* device) {
        m_code = readFile(spvFileName);
        m_module = createShaderModule(m_code, device);
    }

    VulkanShaderModule::~VulkanShaderModule() {}

    VkShaderModule VulkanShaderModule::createShaderModule(
            const std::vector<char>& code, VulkanDevice* device) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device->getLogicalDevice(), &createInfo, 
                    nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Module shader creation failed!");
        }

        return shaderModule;
    }
}
