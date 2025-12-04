#include "material.h"

#include "graphics_environment.h"
#include "render_pass.h"
#include "pipeline.h"
#include "shader.h"
#include "buffer.h"

using namespace HopEngine;
using namespace std;

Material::Material(Ref<Shader> _shader, VkCullModeFlags culling_mode, VkPolygonMode polygon_mode)
{
	shader = _shader;
	pipeline = new Pipeline(shader, culling_mode, polygon_mode, GraphicsEnvironment::get()->getRenderPass()->getRenderPass());

	// TODO: allocate descriptor sets (shader will tell us the layout and size), and buffers, and bind them together
	// TODO: also allocate live buffer
}

Material::~Material()
{
	material_descriptor_sets.clear();
	material_uniform_buffers.clear();
	live_uniform_buffer.clear();
}

VkPipeline Material::getPipeline()
{
	return pipeline->getPipeline();
}

VkPipelineLayout Material::getPipelineLayout()
{
	return shader->getPipelineLayout();
}

void Material::updateUniformBuffer(size_t index)
{
	memcpy(material_uniform_buffers[index]->mapMemory(), live_uniform_buffer.data(), live_uniform_buffer.size());
}
