#include "material.h"

#include <glm/glm.hpp>

#include "graphics_environment.h"
#include "render_pass.h"
#include "pipeline.h"
#include "shader.h"
#include "buffer.h"
#include "uniform_block.h"

using namespace HopEngine;
using namespace std;

Material::Material(Ref<Shader> _shader, VkCullModeFlags culling_mode, VkPolygonMode polygon_mode)
{
	shader = _shader;
	pipeline = new Pipeline(shader, culling_mode, polygon_mode, GraphicsEnvironment::get()->getRenderPass()->getRenderPass());

	uniforms = new UniformBlock(shader->getShaderLayout());
}

Material::~Material()
{
	uniforms = nullptr;
}

VkPipeline Material::getPipeline()
{
	return pipeline->getPipeline();
}

VkPipelineLayout Material::getPipelineLayout()
{
	return shader->getPipelineLayout();
}

void Material::pushToDescriptorSet(size_t index)
{
	uniforms->pushToDescriptorSet(index);
}

VkDescriptorSet Material::getDescriptorSet(size_t index)
{
	return uniforms->getDescriptorSet(index);
}
