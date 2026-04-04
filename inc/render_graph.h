#pragma once

#include <map>
#include <vector>
#include <set>
#include <string>
#include <glm/vec2.hpp>

#include "common.h"
#include "texture.h"
#include "swapchain.h"
#include "engine.h"
#include "material.h"

namespace HopEngine
{

class RenderGraph final : public Destructible
{
public:
	struct AttachmentBinding final
	{
		size_t step_index = 0;
		size_t output_index = 0;
		Sampler::Filter filter_mode = Sampler::FILTER_LINEAR;
		Sampler::Address address_mode = Sampler::ADDRESS_CLAMP_EDGE;
		
		AttachmentBinding() = default;
		AttachmentBinding(const size_t step, const size_t output) : step_index(step), output_index(output) { }
		AttachmentBinding& filter(const Sampler::Filter value) { filter_mode = value; return *this; }
		AttachmentBinding& address(const Sampler::Address value) { address_mode = value; return *this; }
	};

	struct Step final
	{
		bool is_camera = true;
		size_t camera_slot = 0;
		Ref<Material> material;
		std::map<uint32_t, AttachmentBinding> texture_bindings;
		
		float resolution_scale = 1.0f;
		glm::u32vec2 custom_extent{ 0, 0 };
		Ref<RenderPass> render_pass;
		Ref<UniformBlock> scene_uniforms;

		std::string name;
		bool skipped = false;
		
		~Step();
	};

	struct Builder final
	{
		std::vector<Step> execution_steps;
		Sampler::Filter screen_filtering = Sampler::FILTER_LINEAR;

		Builder& addCamera(size_t slot);
		Builder& addCamera(size_t slot, const RenderPass::Config& render_pass_config, float size_factor = 1.0f, glm::u32vec2 custom_extent = { 128, 128 });
		Builder& addCamera(size_t slot, float size_factor, glm::u32vec2 custom_extent = { 128, 128 });
		Builder& addPostProcess(const Ref<Shader>& shader, const std::map<uint32_t, AttachmentBinding>& texture_bindings);
		Builder& addPostProcess(const Ref<Shader>& shader, const std::map<uint32_t, AttachmentBinding>& texture_bindings, const RenderPass::Config& render_pass_config, float size_factor = 1.0f, glm::u32vec2 custom_extent = { 128, 128 });
		Builder& addPostProcess(const Ref<Shader>& shader, const std::map<uint32_t, AttachmentBinding>& texture_bindings, float size_factor, glm::u32vec2 custom_extent = { 128, 128 });
		Builder& filtering(const Sampler::Filter value) { screen_filtering = value; return *this; }
	};


public:
	int output_step = -1;
	int output_image = 0;

private:
	std::string origin;
	std::vector<Step> execution_steps;
	glm::u32vec2 expected_extent = { 0, 0 };
	Ref<Material> passthrough;
	WeakRef<Texture> passthrough_texture;

public:
	DELETE_CONSTRUCTORS(RenderGraph);
	RenderGraph(const Builder& config);
	~RenderGraph() override;
	
	std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }
	WeakRef<Material> getMaterialForStep(size_t step);
	WeakRef<Material> getMaterialForStep(const std::string& name);
	WeakRef<Texture> getFinalImage() const;
	bool getSkipStep(size_t step) const;
	bool getSkipStep(const std::string& name) const;
	void setSkipStep(size_t step, bool skip);
	void setSkipStep(const std::string& name, bool skip);
	
	void resizeBuffers(glm::u32vec2 new_extent);
	void draw(WeakRef<DrawCommandBuffer> command_buffer, const std::vector<DrawCommand>& draw_commands, const std::map<size_t, std::pair<WeakRef<UniformBlock>, glm::vec4>>& cameras);
	void bindOutputMaterial(WeakRef<DrawCommandBuffer> command_buffer);

	std::map<size_t, glm::u32vec2> getCameraSlots();
	
	void drawImGuiDebug();
	static Ref<RenderGraph> deserialise(const std::string& name);
	
private:
	size_t findStep(const std::string& name) const;
	void rebuildBindings();
	static void recordCameraStep(WeakRef<DrawCommandBuffer> command_buffer, const WeakRef<UniformBlock>& camera, glm::vec4 clear_colour, const WeakRef<RenderPass>& pass, const std::multiset<DrawCommand, DrawCommand>& commands);
	static void recordPostProcessStep(WeakRef<DrawCommandBuffer> command_buffer, const WeakRef<Material>& material, const WeakRef<UniformBlock>& scene_descriptor_set);
};

}

// TODO: build a graphical node editor for this....
