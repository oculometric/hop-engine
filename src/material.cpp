#include "material.h"

#include <glm/glm.hpp>
#include <vulkan/vk_enum_string_helper.h>

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
	pipeline = new Pipeline(shader, culling_mode, polygon_mode, GraphicsEnvironment::get()->getRenderPass());

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

	DBG_INFO("created material from shader " + PTR(shader.get()) + " with config " + string(string_VkCullModeFlagBits((VkCullModeFlagBits)culling_mode)) + ", " + string_VkPolygonMode(polygon_mode));
}

Material::~Material()
{
	DBG_INFO("destroying material " + PTR(this));
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
	DBG_BABBLE("material " + PTR(this) + " pushing to descriptor set " + to_string(index));
	uniforms->pushToDescriptorSet(index);
}

VkDescriptorSet Material::getDescriptorSet(size_t index)
{
	return uniforms->getDescriptorSet(index);
}

void Material::setTexture(uint32_t binding, Ref<Texture> texture)
{
	DBG_VERBOSE("material " + PTR(this) + " assigned texture " + PTR(texture.get()) + " to binding " + to_string(binding));
	uniforms->setTexture(binding, texture);
}

void Material::setSampler(uint32_t binding, Ref<Sampler> sampler)
{
	DBG_VERBOSE("material " + PTR(this) + " assigned sampler " + PTR(sampler.get()) + " to binding " + to_string(binding));
	uniforms->setSampler(binding, sampler);
}

void Material::setTexture(string name, Ref<Texture> texture)
{
	auto it = texture_name_to_binding.find(name);
	if (it != texture_name_to_binding.end())
	{
		DBG_VERBOSE("material " + PTR(this) + " assigned texture " + PTR(texture.get()) + " to binding " + name);
		uniforms->setTexture(it->second, texture);
	}
	else
		DBG_WARNING("material " + PTR(this) + " has no such binding " + name);
}

void Material::setSampler(string name, Ref<Sampler> sampler)
{
	auto it = texture_name_to_binding.find(name);
	if (it != texture_name_to_binding.end())
	{
		DBG_VERBOSE("material " + PTR(this) + " assigned sampler " + PTR(sampler.get()) + " to binding " + name);
		uniforms->setSampler(it->second, sampler);
	}
	else
		DBG_WARNING("material " + PTR(this) + " has no such binding " + name);
}

void Material::setUniform(string name, void* data, size_t size)
{
	auto it = variable_name_to_binding.find(name);
	if (it == variable_name_to_binding.end())
	{
		DBG_WARNING("material " + PTR(this) + " has no such uniform " + name);
		return;
	}
	UniformVariable var = it->second;
	if (size != var.size)
		DBG_WARNING("material " + PTR(this) + " uniform " + name + " size mismatch (given " + to_string(size) + ", expected " + to_string(var.size) + ")");
	size_t clamped_size = min(size, var.size);
	memcpy(((uint8_t*)uniforms->getBuffer()) + var.offset, data, clamped_size);
	DBG_VERBOSE("material " + PTR(this) + " updated uniform " + name);
}
