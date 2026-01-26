#pragma once

#include <vector>
#include <glm/vec2.hpp>

#include "common.h"
#include "vulkan_typedefs.h"

namespace HopEngine
{

struct RenderOutput
{
	size_t additional_attachments = 0;
	bool has_depth_attachment = true;
};

class RenderPass : public Destructible
{
private:
	VkRenderPass render_pass = VK_NULL_HANDLE;
	RenderOutput output_config;
	Ref<Texture> depth_texture;
	std::vector<Ref<Texture>> additional_textures;
	std::vector<VkFramebuffer> framebuffers;
	glm::u32vec2 extent;
	Ref<Swapchain> swapchain;

public:
	DELETE_CONSTRUCTORS(RenderPass);

	RenderOutput getOutputConfig() const { return output_config; }
	VkRenderPass getRenderPass() const { return render_pass; }
	VkFramebuffer getFramebuffer(size_t index) const { return framebuffers[index % framebuffers.size()]; }
	std::vector<VkClearValue> getClearValues() const;
	void resize(uint32_t width = 0, uint32_t height = 0);
	glm::u32vec2 getExtent() const { return extent; }
	Ref<Texture> getImage(size_t attachment) const;
	Ref<RenderPass> duplicate() const;
	bool isCompatible(const Ref<RenderPass>& other) const;

	RenderPass(Ref<Swapchain> swapchain, RenderOutput config);
	RenderPass(uint32_t width, uint32_t height, RenderOutput config);
	~RenderPass() override;
	
private:
	void createRenderPass(VkFormat main_colour_format, VkImageLayout final_main_colour_layout, bool make_readable);
	void destroyResources();
	void createResources(Ref<Swapchain> swapchain);
	void createResources(VkFormat main_colour_format, uint32_t width, uint32_t height);
};

}