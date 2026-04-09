#include "render_server.h"
#include "texture.h"
#include "vulkan_helpers.h"

#include <vulkan/vulkan.hpp>

using namespace HopEngine;

static constexpr VkFilter vulkan_filter[2] = { VK_FILTER_NEAREST, VK_FILTER_LINEAR };

TO_STRING_IMPL(Sampler::Filter, 2, VARGS("NEAREST", "LINEAR"));

static constexpr VkSamplerAddressMode vulkan_address[3] = { VK_SAMPLER_ADDRESS_MODE_REPEAT,
    VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE };

TO_STRING_IMPL(Sampler::Address, 3, VARGS("REPEAT", "MIRRORED", "CLAMP_EDGE"));

Sampler::Sampler(Filter filtering_mode, Address address_mode)
{
    VkSamplerCreateInfo create_info{};
    create_info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.magFilter    = vulkan_filter[filtering_mode];
    create_info.minFilter    = vulkan_filter[filtering_mode];
    create_info.addressModeU = vulkan_address[address_mode];
    create_info.addressModeV = vulkan_address[address_mode];
    create_info.addressModeW = vulkan_address[address_mode];
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(static_cast<VkPhysicalDevice>(RenderServer::getPhysicalDevice()),
        &properties);
    create_info.anisotropyEnable        = VK_TRUE;
    create_info.maxAnisotropy           = properties.limits.maxSamplerAnisotropy;
    create_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    create_info.unnormalizedCoordinates = VK_FALSE;
    create_info.compareEnable           = VK_FALSE;
    create_info.compareOp               = VK_COMPARE_OP_ALWAYS;
    create_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    create_info.mipLodBias              = 0.0f;
    create_info.minLod                  = 0.0f;
    create_info.maxLod                  = 0.0f;
    CHECK_RESULT(vkCreateSampler,
        (static_cast<VkDevice>(RenderServer::getDevice()), &create_info, nullptr,
            reinterpret_cast<VkSampler*>(&sampler)),
        FAULT,
        ;);

    DBG_VERBOSE("created sampler for " + to_string(filtering_mode) + ", " + to_string(address_mode));
}

Sampler::~Sampler()
{
    DBG_VERBOSE("destroying sampler " + PTR(this));
    QUEUE_FREE(sampler, VkSampler, vkDestroySampler);
}
