#include "sampler.h"

#include <vulkan/vulkan.hpp>

#include "graphics_environment.h"

using namespace HopEngine;
using namespace std;

static constexpr VkFilter vulkan_filter[2] = 
{
	VK_FILTER_NEAREST,
	VK_FILTER_LINEAR
};

TO_STRING_DEF(SamplerFilter, 2, VARGS("NEAREST", "LINEAR"));

static constexpr VkSamplerAddressMode vulkan_address[3] = 
{
	VK_SAMPLER_ADDRESS_MODE_REPEAT,
	VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
	VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
};

TO_STRING_DEF(SamplerAddress, 3, VARGS("REPEAT", "MIRRORED", "CLAMP_EDGE"));

Sampler::Sampler(const SamplerBuilder& config)
{
	reconfigure(config);
	DBG_VERBOSE("created sampler for " + to_string(config.filtering_mode) + ", " + to_string(config.address_mode));
}

Sampler::~Sampler()
{
	DBG_VERBOSE("destroying sampler " + PTR(this));
	vkDestroySampler(RenderServer::getDevice(), sampler, nullptr);
}

void Sampler::reconfigure(const SamplerBuilder& config)
{
	if (sampler)
		vkDestroySampler(RenderServer::getDevice(), sampler, nullptr);
	
	builder = config;
	VkSamplerCreateInfo create_info{ };
	create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	create_info.magFilter = vulkan_filter[config.filtering_mode];
	create_info.minFilter = vulkan_filter[config.filtering_mode];
	create_info.addressModeU = vulkan_address[config.address_mode];
	create_info.addressModeV = vulkan_address[config.address_mode];
	create_info.addressModeW = vulkan_address[config.address_mode];
	VkPhysicalDeviceProperties properties{ };
	vkGetPhysicalDeviceProperties(RenderServer::getPhysicalDevice(), &properties);
	create_info.anisotropyEnable = VK_TRUE;
	create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	create_info.unnormalizedCoordinates = VK_FALSE;
	create_info.compareEnable = VK_FALSE;
	create_info.compareOp = VK_COMPARE_OP_ALWAYS;
	create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	create_info.mipLodBias = 0.0f;
	create_info.minLod = 0.0f;
	create_info.maxLod = 0.0f;
	if (vkCreateSampler(RenderServer::getDevice(), &create_info, nullptr, &sampler) != VK_SUCCESS)
		DBG_FAULT("vkCreateSampler failed");
}
