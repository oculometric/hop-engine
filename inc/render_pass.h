#pragma once

#include <vector>
#include <glm/vec2.hpp>

#include "common.h"
#include "vulkan_typedefs.h"
#include "texture.h"

namespace HopEngine
{

class RenderPass final : public Destructible
{
public:
	struct Config final
	{
		size_t additional_attachments = 0;
		bool has_depth_attachment = true;
		Texture::Format main_colour_format = Texture::FORMAT_FLOAT_16X4;
	};

private:
	VkRenderPass render_pass = VK_NULL_HANDLE;
	Config output_config;
	std::vector<VkFramebuffer> framebuffers;
	std::vector<Ref<Texture>> textures;
	glm::u32vec2 extent;
	WeakRef<Swapchain> swapchain;

public:
	DELETE_CONSTRUCTORS(RenderPass);
	RenderPass(const WeakRef<Swapchain>& _swapchain, const Config& config);
	RenderPass(glm::u32vec2 image_extent, const Config& config);
	~RenderPass() override;
	
	VkRenderPass getRenderPass() const { return render_pass; }
	Config getOutputConfig() const { return output_config; }
	Ref<Texture> getImage(size_t attachment) const;
	std::vector<VkClearValue> getClearValues() const;
	glm::u32vec2 getExtent() const { return extent; }
	bool isCompatible(const WeakRef<RenderPass>& other) const;
	Ref<RenderPass> duplicate() const;
	void resize(glm::u32vec2 new_extent = { 0, 0 });
	void begin(WeakRef<DrawCommandBuffer> command_buffer, glm::vec3 clear_colour, bool transparent = false);
	
private:
	void createRenderPass();
	void createResources();
	void destroyResources();
};

}