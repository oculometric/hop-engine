#include "sampler.h"

#include <stdexcept>
#include <vulkan/vulkan_to_string.hpp>

#include "graphics_environment.h"

using namespace HopEngine;
using namespace std;

Sampler::Sampler(SamplerBuilder config)
{
	builder = config;
	VkSamplerCreateInfo create_info{ };
	create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	create_info.magFilter = config.filtering_mode;
	create_info.minFilter = config.filtering_mode;
	create_info.addressModeU = config.address_mode;
	create_info.addressModeV = config.address_mode;
	create_info.addressModeW = config.address_mode;
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

	DBG_INFO("created sampler for " + vk::to_string((vk::Filter)config.filtering_mode) + ", " + vk::to_string((vk::SamplerAddressMode)config.address_mode));
}

Sampler::~Sampler()
{
	DBG_INFO("destroying sampler " + PTR(this));
	vkDestroySampler(RenderServer::getDevice(), sampler, nullptr);
}
