#include "material.h"
#include "render_server.h"
#include "vulkan_helpers.h"

#include <shaderc/shaderc.hpp>
#include <spirv_reflect/spirv_reflect.h>
#include <vulkan/vulkan.hpp>

using namespace HopEngine;

std::vector<std::pair<Shader::Stage, GPUHandle>> Shader::getShaderStages() const
{
    return {
        {   STAGE_VERTEX, vert_module },
        { STAGE_FRAGMENT, frag_module }
    };
}

std::vector<Shader::DescriptorBinding> Shader::getReflectedBindings(const std::vector<uint32_t>& blob)
{
    SpvReflectShaderModule reflected_module;
    SpvReflectResult result = spvReflectCreateShaderModule(blob.size() * 4, blob.data(), &reflected_module);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        DBG_WARNING("unable to construct reflection module");
        return {};
    }
    const SpvReflectDescriptorSet* vert_material_set =
        spvReflectGetDescriptorSet(&reflected_module, 2, &result);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        DBG_VERBOSE("unable to reflect descriptor set 2");
        spvReflectDestroyShaderModule(&reflected_module);
        return {};
    }

    std::vector<DescriptorBinding> bindings;
    for (size_t i = 0; i < vert_material_set->binding_count; ++i)
    {
        SpvReflectDescriptorBinding* binding = vert_material_set->bindings[i];
        if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
            DescriptorBinding db;
            db.type        = UNIFORM;
            db.binding     = binding->binding;
            db.buffer_size = binding->block.padded_size;
            db.name        = binding->name;
            for (size_t j = 0; j < binding->block.member_count; ++j)
            {
                SpvReflectBlockVariable member = binding->block.members[j];
                db.variables.push_back({ member.name, member.size, member.offset });
            }
            bindings.push_back(db);
        }
        else if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        {
            DescriptorBinding db;
            db.type    = TEXTURE;
            db.binding = binding->binding;
            if (binding->image.dim == SpvDim::SpvDim3D) db.texture_is_3d = true;
            else if (binding->image.dim != SpvDim::SpvDim2D)
                DBG_WARNING("shader contains an unsupported image sampler dimension.");
            db.name = binding->name;
            bindings.push_back(db);
        }
    }

    spvReflectDestroyShaderModule(&reflected_module);
    return bindings;
}

GPUHandle Shader::createShaderModule(const std::vector<uint32_t>& blob)
{
    VkShaderModuleCreateInfo create_info{};
    create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = blob.size() * 4;
    create_info.pCode    = blob.data();

    VkShaderModule shader_module;
    CHECK_RESULT(vkCreateShaderModule,
        (static_cast<VkDevice>(RenderServer::getDevice()), &create_info, nullptr, &shader_module), ERROR,
        return VK_NULL_HANDLE;);

    return shader_module;
}

std::string formatShaderCode(const std::string& code)
{
    size_t insert_point = 0;
    size_t line_number  = 1;
    std::string result  = code;
    while (insert_point != std::string::npos)
    {
        result.insert(insert_point + 1, std::format("{:>4}| ", line_number));
        ++line_number;
        insert_point = result.find('\n', insert_point + 1);
    }
    return result;
}

bool Shader::compile(const std::string& code, Stage stage, std::vector<uint32_t>& blob,
    const std::string& path)
{
    const shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    shaderc_shader_kind kind;
    if (stage == STAGE_VERTEX)
    {
        options.AddMacroDefinition("vertex", "main");
        kind = shaderc_glsl_vertex_shader;
    }
    else if (stage == STAGE_FRAGMENT)
    {
        options.AddMacroDefinition("fragment", "main");
        kind = shaderc_glsl_fragment_shader;
    }
    else
    {
        DBG_ERROR("attempted to compile a shader with an invalid shader stage parameter");
        return false;
    }

    auto result = compiler.CompileGlslToSpv(code.data(), code.size(), kind, path.c_str(), options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        DBG_ERROR("error compiling vertex shader " + path + ": \n" + result.GetErrorMessage());
        DBG_INFO("see the full shader code below: \n" + formatShaderCode(code));
        return false;
    }

    blob = { result.cbegin(), result.cend() };
    return true;
}

GPUHandle Shader::createDescriptorSetLayout(std::vector<DescriptorBinding> bindings)
{
    VkDescriptorSetLayout layout;
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
    for (const DescriptorBinding& binding : bindings)
    {
        VkDescriptorSetLayoutBinding layout_binding{};
        layout_binding.binding            = binding.binding;
        layout_binding.descriptorType     = (binding.type == UNIFORM)
                                                ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                                : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layout_binding.descriptorCount    = 1;
        layout_binding.pImmutableSamplers = nullptr;
        layout_binding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        layout_bindings.push_back(layout_binding);
    }

    VkDescriptorSetLayoutCreateInfo set_layout_create_info{};
    set_layout_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_layout_create_info.bindingCount = static_cast<uint32_t>(layout_bindings.size());
    set_layout_create_info.pBindings    = layout_bindings.data();

    CHECK_RESULT(vkCreateDescriptorSetLayout,
        (static_cast<VkDevice>(RenderServer::getDevice()), &set_layout_create_info, nullptr, &layout),
        ERROR, return VK_NULL_HANDLE;);

    return layout;
}

void Shader::destroyResources()
{
    QUEUE_FREE(pipeline_layout, VkPipelineLayout, vkDestroyPipelineLayout);
    QUEUE_FREE(descriptor_set_layout, VkDescriptorSetLayout, vkDestroyDescriptorSetLayout);

    QUEUE_FREE(vert_module, VkShaderModule, vkDestroyShaderModule);
    QUEUE_FREE(frag_module, VkShaderModule, vkDestroyShaderModule);
}
