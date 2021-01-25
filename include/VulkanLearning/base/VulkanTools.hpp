#pragma once

#include <cassert>
#include <string>
#include <iostream>
#include <fstream>

#include "vulkan/vulkan.h"

#define VK_CHECK_RESULT(f)																				\
{																										\
    VkResult res = (f);																					\
    if (res != VK_SUCCESS)																				\
    {																									\
        std::cout << "Fatal : VkResult is \"" << VulkanLearning::tools::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
        assert(res == VK_SUCCESS);																		\
    }																									\
}

namespace VulkanLearning {

    namespace tools {

        std::string errorString(VkResult errorCode);

        bool fileExists(const std::string &filename);

        void exitFatal(const std::string& message, int32_t exitCode);
        void exitFatal(const std::string& message, VkResult resultCode);

        // Put an image memory barrier for setting an image layout on the sub resource into the given command buffer
        void setImageLayout(
                VkCommandBuffer cmdbuffer,
                VkImage image,
                VkImageLayout oldImageLayout,
                VkImageLayout newImageLayout,
                VkImageSubresourceRange subresourceRange,
                VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        // Uses a fixed sub resource layout with first mip level and layer
        void setImageLayout(
                VkCommandBuffer cmdbuffer,
                VkImage image,
                VkImageAspectFlags aspectMask,
                VkImageLayout oldImageLayout,
                VkImageLayout newImageLayout,
                VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        /** @brief Insert an image memory barrier into the command buffer */
        void insertImageMemoryBarrier(
                VkCommandBuffer cmdbuffer,
                VkImage image,
                VkAccessFlags srcAccessMask,
                VkAccessFlags dstAccessMask,
                VkImageLayout oldImageLayout,
                VkImageLayout newImageLayout,
                VkPipelineStageFlags srcStageMask,
                VkPipelineStageFlags dstStageMask,
                VkImageSubresourceRange subresourceRange);

    }
}

