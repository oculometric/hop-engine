#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

#include "common.h"

namespace HopEngine
{

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
	Ref<Texture> depth_texture;
	std::vector<Ref<Texture>> additional_textures;
	std::vector<VkFramebuffer> framebuffers;
	VkExtent2D extent;
	Ref<Swapchain> swapchain;

public:
	DELETE_CONSTRUCTORS(RenderPass);

	RenderPass(Ref<Swapchain> swapchain, RenderOutput config);
	RenderPass(uint32_t width, uint32_t height, RenderOutput config);
	~RenderPass();

	inline RenderOutput getOutputConfig() { return output_config; }
	inline VkRenderPass getRenderPass() { return render_pass; }
	inline VkFramebuffer getFramebuffer(size_t index) { return framebuffers[index]; }
	std::vector<VkClearValue> getClearValues();
	void resize(uint32_t width = 0, uint32_t height = 0);
	inline VkExtent2D getExtent() { return extent; }
	Ref<Texture> getImage(size_t attachment);

private:
	void createRenderPass(VkFormat main_colour_format, VkImageLayout final_main_colour_layout, bool make_readable);
	void destroyResources();
	void createResources(Ref<Swapchain> swapchain);
	void createResources(VkFormat main_colour_format, uint32_t width, uint32_t height);
};

}