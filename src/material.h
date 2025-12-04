#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "common.h"

namespace HopEngine
{

// shader uniform layout:
// set 0 -> scene uniforms (time, world to view, view to clip): 1 per frame-in-flight (managed by the environment)
// set 1 -> object uniforms (object id, object to world): 1 per object, per frame-in-flight (managed by the object)
// set 2 -> material uniforms (these are customisable): 1 per material, per frame-in-flight (managed by the material)

// the shader tells us about the layout, but the first 2 sets will NOT be read from the shader
// we create the layouts for the first two sets when the environment loads, since they are the same for all shaders
class Shader;
class Buffer;
class Pipeline;

class Material
{
private:
	Ref<Shader> shader = nullptr;
	Ref<Pipeline> pipeline = nullptr;
	std::vector<VkDescriptorSet> material_descriptor_sets;
	std::vector<Ref<Buffer>> material_uniform_buffers;
	std::vector<uint8_t> live_uniform_buffer;

public:
	DELETE_CONSTRUCTORS(Material);

	Material(Ref<Shader> _shader, VkCullModeFlags culling_mode, VkPolygonMode polygon_mode);
	~Material();

	VkPipeline getPipeline();
	VkPipelineLayout getPipelineLayout();
	void updateUniformBuffer(size_t index);
	// TODO: the material needs to know about the layout/arrangement of variables in the UBO, and have functions for updating them by name
};

}
