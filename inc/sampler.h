#pragma once

#include "common.h"
#include "vulkan_typedefs.h"

namespace HopEngine
{

enum SamplerFilter
{
	FILTER_NEAREST,
	FILTER_LINEAR
};
TO_STRING_DEC(SamplerFilter);

enum SamplerAddress
{
	ADDRESS_REPEAT,
	ADDRESS_MIRRORED,
	ADDRESS_CLAMP_EDGE
};
TO_STRING_DEC(SamplerAddress);

struct SamplerBuilder
{
	SamplerFilter filtering_mode = FILTER_LINEAR;
	SamplerAddress address_mode = ADDRESS_REPEAT;

	SamplerBuilder& filter(const SamplerFilter value) { filtering_mode = value; return *this; }
	SamplerBuilder& address(const SamplerAddress value) { address_mode = value; return *this; }
};

inline bool operator<(const SamplerBuilder& a, const SamplerBuilder& b)
{
	if (a.address_mode < b.address_mode)
		return true;
	if (a.filtering_mode < b.filtering_mode)
		return true;
	return false;
}

class Sampler : public Destructible
{	
private:
	VkSampler sampler = VK_NULL_HANDLE;

public:
	DELETE_CONSTRUCTORS(Sampler);
	Sampler(const SamplerBuilder& config = SamplerBuilder());
	~Sampler() override;
	
	VkSampler getSampler() const { return sampler; }
};

}
