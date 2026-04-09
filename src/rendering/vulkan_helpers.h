#pragma once

#include "texture.h"

#include <array>
#include <vulkan/vulkan.hpp>

namespace HopEngine
{

VkFormat toVulkanFormat(Texture::Format format);
VkImageLayout toVulkanLayout(Texture::Layout layout);

#define CHECK_RESULT(call, args, err, failcode)                                          \
    {                                                                                    \
        VkResult _result = call args;                                                    \
        if (_result != VK_SUCCESS)                                                       \
        {                                                                                \
            DBG_##err(#call " failed with error " + vk::to_string((vk::Result)_result)); \
            failcode;                                                                    \
        }                                                                                \
    }

/**
 * @brief fetches the binding description of the vertex structure.
 * @returns VkVertexInputBindingDescription.
 */
VkVertexInputBindingDescription getVertexBindingDescription();
/**
 * @brief fetches the binding descriptions of each vertex attribute.
 * @returns array of attribute descriptions for the vertex structure.
 */
std::array<VkVertexInputAttributeDescription, 5> getVertexAttributeDescriptions();

} // namespace HopEngine