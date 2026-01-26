#pragma once

#include <vulkan/vulkan.hpp>

#include "common.h"

namespace HopEngine
{

struct SamplerBuilder
{
	VkFilter filtering_mode = VK_FILTER_LINEAR;
	VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	SamplerBuilder filter(VkFilter value) { filtering_mode = value; return *this; }
	SamplerBuilder address(VkSamplerAddressMode value) { address_mode = value; return *this; }
};

class Sampler : public Destructible
{
private:
	VkSampler sampler = VK_NULL_HANDLE;
	SamplerBuilder builder;

public:
	DELETE_CONSTRUCTORS(Sampler);

	VkSampler getSampler() const { return sampler; }
	SamplerBuilder getBuilder() const { return builder; }
	bool drawImGuiDebug();
	
	Sampler(SamplerBuilder config);
	~Sampler() override;
};

}
