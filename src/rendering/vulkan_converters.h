#pragma once

#include <vulkan/vulkan.hpp>

#include "texture.h"

namespace HopEngine
{

VkFormat toVulkanFormat(Texture::Format format);
VkImageLayout toVulkanLayout(Texture::Layout layout);

}