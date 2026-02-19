#pragma once

#include <vector>
#include <glm/vec2.hpp>

#include "common.h"
#include "vulkan_typedefs.h"
#include "texture.h"

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
	std::vector<VkFramebuffer> framebuffers;
	Ref<Texture> depth_texture;
	std::vector<Ref<Texture>> additional_textures;
	glm::u32vec2 extent;
	Ref<Swapchain> swapchain;

public:
	DELETE_CONSTRUCTORS(RenderPass);
	RenderPass(const Ref<Swapchain>& _swapchain, const RenderOutput& config);
	RenderPass(uint32_t width, uint32_t height, const RenderOutput& config);
	~RenderPass() override;
	
	VkRenderPass getRenderPass() const { return render_pass; }
	RenderOutput getOutputConfig() const { return output_config; }
	Ref<Texture> getImage(size_t attachment) const;
	std::vector<VkClearValue> getClearValues() const;
	glm::u32vec2 getExtent() const { return extent; }
	bool isCompatible(const Ref<RenderPass>& other) const;
	Ref<RenderPass> duplicate() const;
	void resize(uint32_t width = 0, uint32_t height = 0);
	void begin(Ref<DrawCommandBuffer> command_buffer, glm::vec3 clear_colour);
	
private:
	void createRenderPass(ImageFormat main_colour_format, ImageLayout final_main_colour_layout, bool make_readable);
	void createResources();
	void createResources(ImageFormat main_colour_format, uint32_t width, uint32_t height);
	void destroyResources();
};

}