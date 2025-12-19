#pragma once

#include <map>
#include <vector>
#include <set>
#include <vulkan/vulkan.hpp>

// the render graph manages render passes, and binds cameras to them
// the render graph binds textures to post processing materials
// the render graph generates commands/executes them in order
// draw commands issued by objects specify which cameras should draw the object (we should check if the camera's render pass is compatible with the material's)
// there are two types of steps in the graph - camera, post-process
// render pass should have a duplicate command, which maintains its structure while creating additional buffers
// all cameras use duplicates of the standard offscreen render pass (colour, depth, four additional)
// post-process steps are free to use their own render passes

#include "common.h"
#include "render_pass.h"
#include "draw_command.h"
#include "engine.h"
#include "uniform_block.h"

namespace HopEngine
{

struct RenderTextureBinding
{
	size_t step_index = 0;
	size_t output_index = 0;
	VkFilter filter_mode = VK_FILTER_LINEAR;
	VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	
	inline RenderTextureBinding() = default;
	inline RenderTextureBinding(size_t step, size_t output) : step_index(step), output_index(output) { }
	inline RenderTextureBinding filter(VkFilter value) { filter_mode = value; return *this; }
	inline RenderTextureBinding address(VkSamplerAddressMode value) { address_mode = value; return *this; }
};

struct RenderStep
{
	bool is_camera = true;
	size_t camera_slot = 0;
	Ref<Material> material;
	std::map<uint32_t, RenderTextureBinding> texture_bindings;
	
	float resolution_scale = 1.0f;
	VkExtent2D custom_extent{ 0, 0 };
	Ref<RenderPass> render_pass;
	Ref<UniformBlock> scene_uniforms;

	~RenderStep();
};

struct RenderGraphBuilder
{
	std::vector<RenderStep> execution_steps;

	RenderGraphBuilder addCamera(size_t slot);
	RenderGraphBuilder addCamera(size_t slot, RenderOutput render_pass_config, float size_factor = 1.0f, VkExtent2D custom_extent = { 128, 128 });
	RenderGraphBuilder addCamera(size_t slot, float size_factor, VkExtent2D custom_extent = { 128, 128 });
	RenderGraphBuilder addPostProcess(Ref<Shader> shader, std::map<uint32_t, RenderTextureBinding> texture_bindings);
	RenderGraphBuilder addPostProcess(Ref<Shader> shader, std::map<uint32_t, RenderTextureBinding> texture_bindings, RenderOutput render_pass_config, float size_factor = 1.0f, VkExtent2D custom_extent = { 128, 128 });
	RenderGraphBuilder addPostProcess(Ref<Shader> shader, std::map<uint32_t, RenderTextureBinding> texture_bindings, float size_factor, VkExtent2D custom_extent = { 128, 128 });
};

class RenderGraph : public Destructible
{
public:
	int output_step = -1;
	int output_image = 0;

private:
	std::vector<RenderStep> execution_steps;
	VkExtent2D expected_extent = { 0, 0 };
	Ref<Material> passthrough;
	WeakRef<Texture> passthrough_texture;

public:
	DELETE_CONSTRUCTORS(RenderGraph);

	RenderGraph(RenderGraphBuilder config);
	~RenderGraph() override;

	void updateUniforms(uint32_t image_index, float time_since_start, Ref<Scene> scene);
	void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index, Ref<Scene> scene, FrameStats& stats, Ref<RenderPass>
	                         final_render_pass) const;
	void resizeBuffers(uint32_t width, uint32_t height);
	inline VkExtent2D getExpectedExtent() const { return expected_extent; }
	Ref<Texture> getFinalImage() const;
	Ref<Material> getMaterialForStep(size_t step);

	void drawImGuiDebug();
	
	static Ref<RenderGraph> deserialise(std::string name);

private:
	void recordCameraStep(VkCommandBuffer command_buffer, uint32_t image_index, Ref<Camera> camera, Ref<RenderPass> pass, std::multiset<DrawCommand, DrawCommand> commands, FrameStats& stats) const;
	void recordPostProcessStep(VkCommandBuffer command_buffer, uint32_t image_index, Ref<Material> material, VkDescriptorSet scene_descriptor_set, FrameStats& stats) const;
};

}

// TODO: build a graphical node editor for this....
