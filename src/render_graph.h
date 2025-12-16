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

namespace HopEngine
{

struct RenderStep
{
	bool is_camera = true;
	size_t camera_slot = 0;
	Ref<Material> material;
	std::map<uint32_t, std::pair<size_t, size_t>> texture_bindings;
	
	Ref<RenderPass> render_pass;
	Ref<UniformBlock> scene_uniforms;
};

struct RenderGraphBuilder
{
	std::vector<RenderStep> execution_steps;

	// TODO: custom render pass size/resolution control
	RenderGraphBuilder addCamera(size_t slot);
	RenderGraphBuilder addCamera(size_t slot, RenderOutput render_pass_config);
	RenderGraphBuilder addPostProcess(Ref<Shader> shader, std::map<uint32_t, std::pair<size_t, size_t>> texture_bindings);
	RenderGraphBuilder addPostProcess(Ref<Shader> shader, RenderOutput render_pass_config, std::map<uint32_t, std::pair<size_t, size_t>> texture_bindings);
};

class RenderGraph
{
private:
	std::vector<RenderStep> execution_steps;
	VkExtent2D expected_extent = { 0, 0 };

public:
	DELETE_CONSTRUCTORS(RenderGraph);

	RenderGraph(RenderGraphBuilder config);

	void updateUniforms(uint32_t image_index, float time_since_start, Ref<Scene> scene);
	void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index, Ref<Scene> scene) const;
	void resizeBuffers(uint32_t width, uint32_t height);
	inline VkExtent2D getExpectedExtent() const { return expected_extent; }
	Ref<Texture> getFinalImage() const; // TODO: control of which image to output 

private:
	void recordCameraStep(VkCommandBuffer command_buffer, uint32_t image_index, Ref<Camera> camera, Ref<RenderPass> pass, std::multiset<DrawCommand, DrawCommand> commands) const;
	void recordPostProcessStep(VkCommandBuffer command_buffer, uint32_t image_index, Ref<Material> material, VkDescriptorSet scene_descriptor_set) const;
};

}

// TODO: build a graphical node editor for this....
// TODO: build a serialisable structure for this
