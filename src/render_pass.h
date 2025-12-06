#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "common.h"

namespace HopEngine
{

class Swapchain;
class Texture;

struct RenderOutput
{
	size_t additional_attachments = 0;
	bool has_depth_attachment = true;
};

class RenderPass
{
private:
	VkRenderPass render_pass = VK_NULL_HANDLE;
	RenderOutput output_config;
	Ref<Texture> depth_texture = nullptr;
	std::vector<Ref<Texture>> additional_textures;
	std::vector<VkFramebuffer> framebuffers;

public:
	DELETE_CONSTRUCTORS(RenderPass);

	// TODO: render-to-texture initialiser (custom format and size, one framebuffer)
	RenderPass(Ref<Swapchain> swapchain, RenderOutput config);
	~RenderPass();

	// TODO: resizebuffers function
	inline RenderOutput getOutputConfig() { return output_config; }
	inline VkRenderPass getRenderPass() { return render_pass; }
	inline VkFramebuffer getFramebuffer(size_t index) { return framebuffers[index]; }
	std::vector<VkClearValue> getClearValues();
};

}