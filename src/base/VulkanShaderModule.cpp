#include "VulkanShaderModule.hpp"
#include "VulkanTools.hpp"

namespace VulkanLearning {

    VulkanShaderModule::VulkanShaderModule(
            std::string spvFileName, 
            VulkanDevice* device,
            VkShaderStageFlagBits stage) {
        m_code = readFile(spvFileName);

        m_module = createShaderModule(m_code, device, stage);

        m_stageCreateInfo = {};
        m_stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        m_stageCreateInfo.stage = stage;
        m_stageCreateInfo.module = m_module;
        m_stageCreateInfo.pName = "main";
    }

    VulkanShaderModule::~VulkanShaderModule() {}

    VkShaderModule VulkanShaderModule::createShaderModule(
            const std::vector<char>& code, 
            VulkanDevice* device,
            VkShaderStageFlagBits stage) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        VK_CHECK_RESULT(vkCreateShaderModule(
                    device->getLogicalDevice(), 
                    &createInfo, 
                    nullptr, 
                    &shaderModule));

        return shaderModule;
    }

    void VulkanShaderModule::cleanup(VulkanDevice* device) {
        vkDestroyShaderModule(device->getLogicalDevice(), m_module, nullptr);
    }
}
