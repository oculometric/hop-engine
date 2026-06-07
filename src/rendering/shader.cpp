#include "command_buffer.h"
#include "material.h"
#include "render_server.h"

#include <filesystem>

using namespace HopEngine;

Shader::Shader(const std::string& base_path)
{
    origin = base_path;
    std::vector<uint32_t> vert_blob;
    std::vector<uint32_t> frag_blob;
    load_succeeded = Shader::compileShaders(base_path, vert_blob, frag_blob);
    hash           = 0;

    if (!load_succeeded)
    {
        if (!Shader::compileShaders("res://engine/shaders/default_shader.glsl", vert_blob, frag_blob))
        {
            DBG_FAULT("failed to load default shader!");
        }
    }

    hash = Shader::computeHash(vert_blob, frag_blob);

    vert_module = Shader::createShaderModule(vert_blob);
    frag_module = Shader::createShaderModule(frag_blob);

    const auto vert_bindings = Shader::getReflectedBindings(vert_blob);
    const auto frag_bindings = Shader::getReflectedBindings(frag_blob);

    bindings = Shader::mergeBindings(vert_bindings, frag_bindings);

    descriptor_set_layout = Shader::createDescriptorSetLayout(bindings);

    pipeline_layout = RenderServer::createPipelineLayout(descriptor_set_layout);

    DBG_VERBOSE("created shader from " + base_path);
}

Shader::~Shader()
{
    DBG_VERBOSE("destroyed shader '" + getOrigin() + '\'');

    destroyResources();
}

void Shader::bind(WeakRef<DrawCommandBuffer> command_buffer)
{ command_buffer->bindPipelineLayoutInternal(pipeline_layout); }

std::vector<Shader::Descriptor> Shader::mergeBindings(const std::vector<Descriptor>& list_a,
    const std::vector<Descriptor>& list_b)
{
    std::multimap<uint32_t, Descriptor> bindings;

    for (const auto& item : list_a) bindings.insert({ item.binding, item });
    for (const auto& item : list_b) bindings.insert({ item.binding, item });

    if (bindings.empty()) return {};
    if (bindings.size() == 1) return { bindings.begin()->second };

    std::vector<Descriptor> resolved_bindings;

    auto binding_it = bindings.begin();
    while (binding_it != bindings.end())
    {
        Descriptor last_binding = binding_it->second;
        resolved_bindings.push_back(last_binding);
        ++binding_it;
        if (binding_it == bindings.end()) return resolved_bindings;
        if (binding_it->first == last_binding.binding)
        {
            // uh oh! duplicate bindings! that's not good...
            if (binding_it->second.type == last_binding.type &&
                binding_it->second.buffer_size == last_binding.buffer_size)
                ++binding_it;
            else
            {
                DBG_ERROR("incompatible duplicate shader uniform/texture bindings found");
                ++binding_it;
            }
        }
    }

    return resolved_bindings;
}

void Shader::reload()
{
    if (origin.empty()) return;

    std::vector<uint32_t> vert_blob;
    std::vector<uint32_t> frag_blob;
    if (!compileShaders(origin, vert_blob, frag_blob))
    {
        DBG_ERROR("shader '" + origin + "' reload failed");
        return;
    }

    uint64_t new_hash = computeHash(vert_blob, frag_blob);

    if (new_hash == hash)
    {
        DBG_VERBOSE("shader '" + origin + "' was not reloaded, as its hash did not change");
        return;
    }

    destroyResources();
    hash = new_hash;

    vert_module = createShaderModule(vert_blob);
    frag_module = createShaderModule(frag_blob);

    const auto vert_bindings = getReflectedBindings(vert_blob);
    const auto frag_bindings = getReflectedBindings(frag_blob);

    bindings = mergeBindings(vert_bindings, frag_bindings);

    descriptor_set_layout = createDescriptorSetLayout(bindings);

    pipeline_layout = RenderServer::createPipelineLayout(descriptor_set_layout);
}

uint64_t Shader::computeHash(const std::vector<uint32_t>& blob1, const std::vector<uint32_t>& blob2)
{
    uint64_t result = 0xCA55E77E;
    uint8_t segment = 0;
    for (const uint32_t elem : blob1)
    {
        result ^= elem << segment;
        ++segment;
        if (segment > 32) segment = 0;
    }
    for (const uint32_t elem : blob2)
    {
        result ^= elem << segment;
        ++segment;
        if (segment > 32) segment = 0;
    }
    return result;
}
