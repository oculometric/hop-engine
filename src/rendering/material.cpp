#include "material.h"

#include "command_buffer.h"
#include "engine.h"
#include "framebuffer.h"
#include "graphics_server.h"
#include "texture.h"

using namespace HopEngine;

Material::Material(Ref<Shader> _shader, const Pipeline::Builder& config,
    const Framebuffer::Config& _render_pass)
{
    render_config = _render_pass;
    shader        = _shader;
    initaliseMaterial(config);

    DBG_VERBOSE("created material from shader '" + shader->getOrigin() + "' with config " +
                to_string(config.culling_mode) + ", " + to_string(config.polygon_mode));
}

Material::Material(Ref<Shader> _shader, const Pipeline::Builder& config)
{
    render_config = Framebuffer::getDefaultConfig();
    shader        = _shader;
    initaliseMaterial(config);

    DBG_VERBOSE("created material from shader '" + shader->getOrigin() + "' with config " +
                to_string(config.culling_mode) + ", " + to_string(config.polygon_mode));
}

Material::~Material() { DBG_VERBOSE("destroying material '" + getOrigin() + '\''); }

Ref<Shader> Material::getShader() const { return shader; }

Ref<Material> Material::duplicate() const
{ return new Material(shader, pipeline->getConfig(), render_config); }

void Material::bind(WeakRef<DrawCommandBuffer> command_buffer, bool wireframe_allowed)
{
    if (last_known_shader_hash != shader->getHash())
    {
        initaliseMaterial(pipeline->getConfig());
        for (const auto& tex : material_textures) setTexture(tex.first, tex.second);
        for (const auto& sam : material_samplers) setSampler(sam.first, sam.second);
        for (const auto& uni : material_parameters)
            setUniform(uni.first, uni.second.data(), uni.second.size());
    }

    if (Engine::isWireframeMode() && wireframe_allowed) debug_pipeline->bind(command_buffer);
    else
        pipeline->bind(command_buffer);
    shader->bind(command_buffer);
    uniforms->bind(command_buffer);
}

void Material::setTexture(const uint32_t binding, Ref<Texture> texture)
{ uniforms->setTexture(binding, texture); }

void Material::setSampler(const uint32_t binding, Ref<Sampler> sampler)
{ uniforms->setSampler(binding, sampler); }

void Material::setTexture(const std::string& name, Ref<Texture> texture)
{
    if (!shader->didCompileSuccessfully()) return;
    const auto it = texture_name_to_binding.find(name);
    if (it != texture_name_to_binding.end())
    {
        uniforms->setTexture(it->second, texture);
        material_textures[name] = texture;
    }
    else
        DBG_WARNING("material '" + getOrigin() + "' has no such binding '" + name + '\'');
}

void Material::setSampler(const std::string& name, Ref<Sampler> sampler)
{
    if (!shader->didCompileSuccessfully()) return;
    const auto it = texture_name_to_binding.find(name);
    if (it != texture_name_to_binding.end())
    {
        uniforms->setSampler(it->second, sampler);
        material_samplers[name] = sampler;
    }
    else
        DBG_WARNING("material '" + getOrigin() + "' has no such binding '" + name + '\'');
}

void Material::setTextureSampler(uint32_t binding, Ref<Texture> texture, Ref<Sampler> sampler)
{
    setTexture(binding, texture);
    setSampler(binding, sampler);
}

void Material::setTextureSampler(const std::string& name, Ref<Texture> texture, Ref<Sampler> sampler)
{
    setTexture(name, texture);
    setSampler(name, sampler);
}

void Material::setUniform(const std::string& name, const void* data, size_t size)
{
    if (!shader->didCompileSuccessfully()) return;
    const auto it = variable_name_to_binding.find(name);
    if (it == variable_name_to_binding.end())
    {
        DBG_WARNING("material '" + getOrigin() + "' has no such uniform '" + name + '\'');
        return;
    }
    Shader::UniformVariable var = it->second;
    if (size != var.size)
        DBG_WARNING("material '" + getOrigin() + "' uniform '" + name + "' size mismatch (given " +
                    std::to_string(size) + ", expected " + std::to_string(var.size) + ")");
    const size_t clamped_size = glm::min(size, var.size);
    memcpy(static_cast<uint8_t*>(uniforms->getBuffer()) + var.offset, data, clamped_size);
    std::vector<uint8_t> vec(clamped_size);
    memcpy(vec.data(), data, clamped_size);
    material_parameters[name] = vec;
}

void Material::initaliseMaterial(const Pipeline::Builder& config)
{
    last_known_shader_hash = shader->getHash();
    pipeline               = new Pipeline(shader, config, render_config);
    debug_pipeline =
        new Pipeline(shader, Pipeline::Builder().polygonMode(Pipeline::POLYGON_LINE), render_config);

    const auto layout = shader->getShaderLayout();
    uniforms          = new UniformBlock(layout);

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
}
