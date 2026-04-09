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

#define QUEUE_FREE(resource, type, func)                                                                \
    {                                                                                                   \
        auto _temp_##type = resource;                                                                   \
        RenderServer::queueFree(                                                                        \
            [_temp_##type]()                                                                            \
            {                                                                                           \
                func(static_cast<VkDevice>(RenderServer::getDevice()), static_cast<type>(_temp_##type), \
                    nullptr);                                                                           \
            });                                                                                         \
        resource = VK_NULL_HANDLE;                                                                      \
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

template<typename T, typename S, int L> T convertFlags(S usage, const T* mapping)
{
    uint32_t flags = 0;
    for (size_t i = 0; i < L; ++i)
    {
        if (usage & (1 << i)) flags = mapping[i] | flags;
    }
    return static_cast<T>(flags);
}

} // namespace HopEngine