#pragma once

#include <vulkan/vulkan.h>

#include "common.h"

namespace HopEngine
{

class Swapchain;

class RenderPass
{
private:
	VkRenderPass render_pass = VK_NULL_HANDLE;

public:
	DELETE_CONSTRUCTORS(RenderPass);

	RenderPass(Ref<Swapchain> swapchain);
	~RenderPass();

	inline VkRenderPass getRenderPass() { return render_pass; }
};

}