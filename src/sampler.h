#pragma once

#include <vulkan/vulkan.h>

#include "common.h"

namespace HopEngine
{

class Sampler
{
private:
	VkSampler sampler = VK_NULL_HANDLE;

public:
	DELETE_CONSTRUCTORS(Sampler);

	Sampler(VkFilter filtering_mode, VkSamplerAddressMode address_mode);
	~Sampler();

	inline VkSampler getSampler() { return sampler; }
};

}
