#include "material.h"

#include "command_buffer.h"
#include "engine.h"
#include "render_server.h"
#include "render_pass.h"
#include "pipeline.h"
#include "uniform_block.h"
#include "texture.h"

using namespace HopEngine;
using namespace std;

Material::Material(Ref<Shader> _shader, const Pipeline::Builder& config, WeakRef<RenderPass> _render_pass)
{
	render_pass = _render_pass ? _render_pass.strong() : RenderServer::getMainRenderPass().strong();
	shader = _shader;
	pipeline = new Pipeline(shader, config, render_pass);
	debug_pipeline = new Pipeline(shader, Pipeline::Builder().polygonMode(Pipeline::POLYGON_LINE), render_pass);

	const auto layout = shader->getShaderLayout();
	uniforms = new UniformBlock(layout);
	
	for (const auto& binding : layout.bindings)
	{
		if (binding.type == Shader::UNIFORM)
		{
			for (const auto& variable : binding.variables)
				variable_name_to_binding[variable.name] = variable;
		}
		else if (binding.type == Shader::TEXTURE)
			texture_name_to_binding[binding.name] = binding.binding;
	}

	DBG_VERBOSE("created material from shader '" + shader->getOrigin() + "' with config " + to_string(config.culling_mode) + ", " + to_string(config.polygon_mode));
}

Material::~Material()
{
	DBG_VERBOSE("destroying material '" + getOrigin() + '\'');
}

Ref<Shader> Material::getShader() const
{
	return shader;
}

Ref<RenderPass> Material::getRenderPass() const
{
	return render_pass;
}

Ref<Material> Material::duplicate() const
{
	return new Material(shader, pipeline->getConfig(), render_pass);
}

void Material::bind(WeakRef<DrawCommandBuffer> command_buffer, bool wireframe_allowed)
{
	if (Engine::isWireframeMode() && wireframe_allowed)
		debug_pipeline->bind(command_buffer);
	else
		pipeline->bind(command_buffer);
	shader->bind(command_buffer);
	uniforms->bind(command_buffer);
}

void Material::setTexture(const uint32_t binding, Ref<Texture> texture)
{
	DBG_BABBLE("material '" + getOrigin() + "' assigned texture '" + texture->getOrigin() + "' to binding " + ::to_string(binding));
	uniforms->setTexture(binding, texture);
}

void Material::setSampler(const uint32_t binding, Ref<Sampler> sampler)
{
	DBG_BABBLE("material '" + getOrigin() + "' assigned sampler " + PTR(sampler.get()) + " to binding " + ::to_string(binding));
	uniforms->setSampler(binding, sampler);
}

void Material::setTexture(const string& name, Ref<Texture> texture)
{
	const auto it = texture_name_to_binding.find(name);
	if (it != texture_name_to_binding.end())
	{
		DBG_BABBLE("material '" + getOrigin() + "' assigned texture '" + texture->getOrigin() + "' to binding '" + name + '\'');
		uniforms->setTexture(it->second, texture);
	}
	else
		DBG_WARNING("material '" + getOrigin() + "' has no such binding '" + name + '\'');
}

void Material::setSampler(const string& name, Ref<Sampler> sampler)
{
	const auto it = texture_name_to_binding.find(name);
	if (it != texture_name_to_binding.end())
	{
		DBG_BABBLE("material '" + getOrigin() + "' assigned sampler " + PTR(sampler.get()) + " to binding '" + name + '\'');
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
	Shader::UniformVariable var = it->second;
	if (size != var.size)
		DBG_WARNING("material '" + getOrigin() + "' uniform '" + name + "' size mismatch (given " + ::to_string(size) + ", expected " + ::to_string(var.size) + ")");
	const size_t clamped_size = min(size, var.size);
	memcpy(static_cast<uint8_t*>(uniforms->getBuffer()) + var.offset, data, clamped_size);
	DBG_BABBLE("material '" + getOrigin() + "' updated uniform '" + name + '\'');
}
