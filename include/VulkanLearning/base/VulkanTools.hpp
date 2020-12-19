#pragma once

#include <cassert>
#include <string>

#include "vulkan/vulkan.h"

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << VulkanLearning::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}

namespace VulkanLearning {

    std::string errorString(VkResult errorCode);

}

