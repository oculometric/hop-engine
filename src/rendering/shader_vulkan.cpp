#include "material.h"
#include "render_server.h"
#include "vulkan_helpers.h"

#include <format>
#include <Public/ResourceLimits.h>
#include <Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
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

std::vector<Shader::Descriptor> Shader::getReflectedBindings(const std::vector<uint32_t>& blob)
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

    std::vector<Descriptor> bindings;
    for (size_t i = 0; i < vert_material_set->binding_count; ++i)
    {
        SpvReflectDescriptorBinding* binding = vert_material_set->bindings[i];
        if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
            Descriptor db;
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
            Descriptor db;
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
        result.insert(insert_point, std::format("{:>4}| ", line_number));
        ++line_number;
        insert_point = result.find('\n', insert_point + 1);
        if (insert_point != std::string::npos) ++insert_point;
    }
    return result;
}

bool Shader::compile(const std::string& code, Stage stage, std::vector<uint32_t>& blob,
    const std::string& path)
{
    glslang::InitializeProcess();

    EShLanguage stage_language;
    EProfile profile        = ENoProfile;
    std::string shader_type_name = "";
    int version             = 450;
    if (stage == STAGE_VERTEX)
    {
        stage_language = EShLangVertex;
        shader_type_name = "vertex";
    }
    else if (stage == STAGE_FRAGMENT)
    {
        stage_language = EShLangFragment;
        shader_type_name = "fragment";
    }
    else
    {
        DBG_ERROR("attempted to compile a shader with an invalid shader stage parameter");
        return false;
    }

    glslang::TShader shader(stage_language);
    const char* shader_strings = code.data();
    const int shader_lengths   = static_cast<int>(code.size());
    const char* file_name      = path.c_str();
    shader.setStringsWithLengthsAndNames(&shader_strings, &shader_lengths, &file_name, 1);
    shader.setEntryPoint("main");
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
    shader.setOverrideVersion(450);
    bool success = shader.parse(GetDefaultResources(), 110, ENoProfile, false, false,
        static_cast<EShMessages>(EShMsgCascadingErrors | EShMsgSpvRules | EShMsgVulkanRules));
    if (!success)
    {
        DBG_ERROR("error compiling " + shader_type_name + " shader " + path + ": \n" + shader.getInfoLog());
        DBG_INFO("see the full shader code below: \n" + formatShaderCode(code));
        return false;
    }

    glslang::TProgram program;
    program.addShader(&shader);
    success = program.link(EShMsgDefault) && program.mapIO();
    if (!success)
    {
        DBG_ERROR("error linking shader " + path + ": \n" + program.getInfoLog());
        DBG_INFO("see the full shader code below: \n" + formatShaderCode(code));
        return false;
    }

    glslang::SpvOptions options;
    options.optimizeSize = false;
    glslang::GlslangToSpv(*program.getIntermediate(stage_language), blob, &options);

    return true;
}

GPUHandle Shader::createDescriptorSetLayout(std::vector<Descriptor> bindings)
{
    VkDescriptorSetLayout layout;
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
    for (const Descriptor& binding : bindings)
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
