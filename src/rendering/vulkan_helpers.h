#pragma once

#include <vulkan/vulkan.hpp>

#include "texture.h"

namespace HopEngine
{

VkFormat toVulkanFormat(Texture::Format format);
VkImageLayout toVulkanLayout(Texture::Layout layout);

#define CHECK_RESULT(call, args, err, failcode) { \
    VkResult _result = call args; \
    if (_result != VK_SUCCESS) \
    { \
        DBG_##err (#call " failed with error " + vk::to_string((vk::Result)_result)); \
        failcode ; \
    } \
}

}