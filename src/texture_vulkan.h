#pragma once

#include <vulkan/vulkan.hpp>

#include "texture.h"

namespace HopEngine
{

VkFormat toVulkanFormat(ImageFormat format);
VkImageLayout toVulkanLayout(ImageLayout layout);

}