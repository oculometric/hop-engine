#pragma once

#include <map>
#include <vector>
#include <set>
#include <string>
#include <glm/vec2.hpp>

#include "common.h"
#include "vulkan_typedefs.h"
#include "sampler.h"
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
	SamplerFilter filter_mode = FILTER_LINEAR;
	SamplerAddress address_mode = ADDRESS_CLAMP_EDGE;
	
	RenderTextureBinding() = default;
	RenderTextureBinding(const size_t step, const size_t output) : step_index(step), output_index(output) { }
	RenderTextureBinding& filter(const SamplerFilter value) { filter_mode = value; return *this; }
	RenderTextureBinding& address(const SamplerAddress value) { address_mode = value; return *this; }
};

struct RenderStep
{
	bool is_camera = true;
	size_t camera_slot = 0;
	Ref<Material> material;
	std::map<uint32_t, RenderTextureBinding> texture_bindings;
	
	float resolution_scale = 1.0f;
	glm::u32vec2 custom_extent{ 0, 0 };
	Ref<RenderPass> render_pass;
	Ref<UniformBlock> scene_uniforms;

	std::string name;
	bool skipped = false;
	
	~RenderStep();
};

struct RenderGraphBuilder
{
	std::vector<RenderStep> execution_steps;

	RenderGraphBuilder& addCamera(size_t slot);
	RenderGraphBuilder& addCamera(size_t slot, const RenderOutput& render_pass_config, float size_factor = 1.0f, glm::u32vec2 custom_extent = { 128, 128 });
	RenderGraphBuilder& addCamera(size_t slot, float size_factor, glm::u32vec2 custom_extent = { 128, 128 });
	RenderGraphBuilder& addPostProcess(const Ref<Shader>& shader, const std::map<uint32_t, RenderTextureBinding>& texture_bindings);
	RenderGraphBuilder& addPostProcess(const Ref<Shader>& shader, const std::map<uint32_t, RenderTextureBinding>& texture_bindings, const RenderOutput& render_pass_config, float size_factor = 1.0f, glm::u32vec2 custom_extent = { 128, 128 });
	RenderGraphBuilder& addPostProcess(const Ref<Shader>& shader, const std::map<uint32_t, RenderTextureBinding>& texture_bindings, float size_factor, glm::u32vec2 custom_extent = { 128, 128 });
};

class RenderGraph : public Destructible
{
public:
	int output_step = -1;
	int output_image = 0;

private:
	std::string origin;
	std::vector<RenderStep> execution_steps;
	glm::u32vec2 expected_extent = { 0, 0 };
	Ref<Material> passthrough;
	WeakRef<Texture> passthrough_texture;

public:
	DELETE_CONSTRUCTORS(RenderGraph);
	RenderGraph(const RenderGraphBuilder& config);
	~RenderGraph() override;
	
	std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }
	Ref<Material> getMaterialForStep(size_t step);
	Ref<Material> getMaterialForStep(const std::string& name);
	std::pair<Ref<Texture>, bool> getFinalImage() const;
	bool getSkipStep(size_t step) const;
	bool getSkipStep(const std::string& name) const;
	glm::u32vec2 getExpectedExtent() const { return expected_extent; }
	void setSkipStep(size_t step, bool skip);
	void setSkipStep(const std::string& name, bool skip);
	void resizeBuffers(uint32_t width, uint32_t height);
	void updateUniforms(uint32_t image_index, float time_since_start, Ref<Scene> scene);
	void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index, Ref<Scene> scene, FrameStats& stats, Ref<RenderPass>
	                         final_render_pass) const;

	void drawImGuiDebug();
	static Ref<RenderGraph> deserialise(const std::string& name);
	
private:
	size_t findStep(const std::string& name) const;
	void rebuildBindings();
	static void recordCameraStep(VkCommandBuffer command_buffer, uint32_t image_index, const Ref<Camera>& camera, const Ref<RenderPass>& pass, const std::multiset<DrawCommand, DrawCommand>& commands, FrameStats& stats);
	static void recordPostProcessStep(VkCommandBuffer command_buffer, uint32_t image_index, const Ref<Material>& material, VkDescriptorSet scene_descriptor_set, FrameStats& stats);
};

}

// TODO: build a graphical node editor for this....
