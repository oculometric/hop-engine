#include "material.h"

#include <glm/glm.hpp>

#include "graphics_environment.h"
#include "render_pass.h"
#include "pipeline.h"
#include "shader.h"
#include "buffer.h"
#include "uniform_block.h"
#include "sampler.h"
#include "texture.h"

using namespace HopEngine;
using namespace std;

Material::Material(Ref<Shader> _shader, VkCullModeFlags culling_mode, VkPolygonMode polygon_mode)
{
	shader = _shader;
	pipeline = new Pipeline(shader, culling_mode, polygon_mode, GraphicsEnvironment::get()->getRenderPass()->getRenderPass());

	auto layout = shader->getShaderLayout();
	uniforms = new UniformBlock(layout);
	
	for (auto binding : layout.bindings)
	{
		if (binding.type == UNIFORM)
		{
			for (auto variable : binding.variables)
			{
				variable_name_to_binding[variable.name] = variable;
			}
		}
		else if (binding.type == TEXTURE)
			texture_name_to_binding[binding.name] = binding.binding;
	}
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

void Material::setTexture(size_t index, Ref<Image> texture)
{
	uniforms->setTexture(index, texture);
}

void Material::setSampler(size_t index, Ref<Sampler> sampler)
{
	uniforms->setSampler(index, sampler);
}

void Material::setTexture(string name, Ref<Image> texture)
{
	auto it = texture_name_to_binding.find(name);
	if (it != texture_name_to_binding.end())
		uniforms->setTexture(it->second, texture);
}

void Material::setSampler(string name, Ref<Sampler> sampler)
{
	auto it = texture_name_to_binding.find(name);
	if (it != texture_name_to_binding.end())
		uniforms->setSampler(it->second, sampler);
}

void Material::setUniformVariable(string name, void* data, size_t size)
{
	auto it = variable_name_to_binding.find(name);
	if (it == variable_name_to_binding.end())
		return;
	UniformVariable var = it->second;
	size_t clamped_size = min(size, var.size);
	memcpy(((uint8_t*)uniforms->getBuffer()) + var.offset, data, clamped_size);
}
