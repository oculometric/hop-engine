#include "material.h"

#include <vulkan/vulkan.hpp>

#include "graphics_environment.h"
#include "render_pass.h"
#include "pipeline.h"
#include "shader.h"
#include "uniform_block.h"
#include "sampler.h"
#include "texture.h"

using namespace HopEngine;
using namespace std;

Material::Material(const Ref<Shader>& _shader, const PipelineBuilder& config, const Ref<RenderPass>& _render_pass)
{
	render_pass = _render_pass.isValid() ? _render_pass : RenderServer::getMainRenderPass();
	shader = _shader;
	pipeline = new Pipeline(shader, config, render_pass);
	debug_pipeline = new Pipeline(shader, PipelineBuilder().polygonMode(POLYGON_LINE), render_pass);

	const auto layout = shader->getShaderLayout();
	uniforms = new UniformBlock(layout);
	
	for (const auto& binding : layout.bindings)
	{
		if (binding.type == UNIFORM)
		{
			for (const auto& variable : binding.variables)
				variable_name_to_binding[variable.name] = variable;
		}
		else if (binding.type == TEXTURE)
			texture_name_to_binding[binding.name] = binding.binding;
	}

	DBG_INFO("created material from shader '" + shader->getOrigin() + "' with config " + to_string(config.culling_mode) + ", " + to_string(config.polygon_mode));
}

Material::~Material()
{
	DBG_INFO("destroying material '" + getOrigin() + '\'');
	uniforms = nullptr;
	pipeline = nullptr;
	shader = nullptr;
}

Ref<Shader> Material::getShader() const
{
	return shader;
}

VkPipelineLayout Material::getPipelineLayout() const
{
	return shader->getPipelineLayout();
}

VkPipeline Material::getPipeline() const
{
	return pipeline->getPipeline();
}

VkPipeline Material::getDebugPipeline() const
{
	return debug_pipeline->getPipeline();
}

VkDescriptorSet Material::getDescriptorSet(const size_t index) const
{
	return uniforms->getDescriptorSet(index);
}

Ref<RenderPass> Material::getRenderPass() const
{
	return render_pass;
}

void Material::pushToDescriptorSet(const size_t index)
{
	DBG_BABBLE("material '" + getOrigin() + "' pushing to descriptor set " + to_string(index));
	uniforms->pushToDescriptorSet(index);
}

Ref<Material> Material::duplicate() const
{
	return new Material(shader, pipeline->getConfig(), render_pass);
}

void Material::setTexture(const uint32_t binding, const Ref<Texture>& texture, const bool use_stencil)
{
	DBG_VERBOSE("material '" + getOrigin() + "' assigned texture '" + texture->getOrigin() + "' to binding " + to_string(binding));
	uniforms->setTexture(binding, texture, use_stencil);
}

void Material::setSampler(const uint32_t binding, const Ref<Sampler>& sampler)
{
	DBG_VERBOSE("material '" + getOrigin() + "' assigned sampler " + PTR(sampler.get()) + " to binding " + to_string(binding));
	uniforms->setSampler(binding, sampler);
}

void Material::setTexture(const string& name, const Ref<Texture>& texture, const bool use_stencil)
{
	const auto it = texture_name_to_binding.find(name);
	if (it != texture_name_to_binding.end())
	{
		DBG_VERBOSE("material '" + getOrigin() + "' assigned texture '" + texture->getOrigin() + "' to binding '" + name + '\'');
		uniforms->setTexture(it->second, texture, use_stencil);
	}
	else
		DBG_WARNING("material '" + getOrigin() + "' has no such binding '" + name + '\'');
}

void Material::setSampler(const string& name, const Ref<Sampler>& sampler)
{
	const auto it = texture_name_to_binding.find(name);
	if (it != texture_name_to_binding.end())
	{
		DBG_VERBOSE("material '" + getOrigin() + "' assigned sampler " + PTR(sampler.get()) + " to binding '" + name + '\'');
		uniforms->setSampler(it->second, sampler);
	}
	else
		DBG_WARNING("material '" + getOrigin() + "' has no such binding '" + name + '\'');
}

void Material::setUniform(const string& name, const void* data, size_t size)
{
	const auto it = variable_name_to_binding.find(name);
	if (it == variable_name_to_binding.end())
	{
		DBG_WARNING("material '" + getOrigin() + "' has no such uniform '" + name + '\'');
		return;
	}
	UniformVariable var = it->second;
	if (size != var.size)
		DBG_WARNING("material '" + getOrigin() + "' uniform '" + name + "' size mismatch (given " + ::to_string(size) + ", expected " + ::to_string(var.size) + ")");
	const size_t clamped_size = min(size, var.size);
	memcpy(static_cast<uint8_t*>(uniforms->getBuffer()) + var.offset, data, clamped_size);
	DBG_VERBOSE("material '" + getOrigin() + "' updated uniform '" + name + '\'');
}
