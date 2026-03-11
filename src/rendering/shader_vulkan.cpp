#include "material.h"

#include <vulkan/vulkan.hpp>
#include <shaderc/shaderc.hpp>
#include <spirv_reflect/spirv_reflect.h>
#include <filesystem>

#include "package.h"
#include "render_server.h"

using namespace HopEngine;
using namespace std;

vector<VkPipelineShaderStageCreateInfo> Shader::getShaderStageCreateInfos() const
{
	VkPipelineShaderStageCreateInfo vert_stage_create_info{ };
	vert_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_stage_create_info.module = vert_module;
	vert_stage_create_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_stage_create_info{ };
	frag_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_stage_create_info.module = frag_module;
	frag_stage_create_info.pName = "main";

	return { vert_stage_create_info, frag_stage_create_info };
}

vector<Shader::DescriptorBinding> Shader::getReflectedBindings(const vector<uint32_t>& blob)
{
	SpvReflectShaderModule reflected_module;
	SpvReflectResult result = spvReflectCreateShaderModule(blob.size() * 4, blob.data(), &reflected_module);
	if (result != SPV_REFLECT_RESULT_SUCCESS)
	{
		DBG_WARNING("unable to construct reflection module");
		return { };
	}
	const SpvReflectDescriptorSet* vert_material_set = spvReflectGetDescriptorSet(&reflected_module, 2, &result);
	if (result != SPV_REFLECT_RESULT_SUCCESS)
	{
		DBG_VERBOSE("unable to reflect descriptor set 2");
		spvReflectDestroyShaderModule(&reflected_module);
		return { };
	}

	vector<DescriptorBinding> bindings;
	for (size_t i = 0; i < vert_material_set->binding_count; ++i)
	{
		SpvReflectDescriptorBinding* binding = vert_material_set->bindings[i];
		if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			DescriptorBinding db;
			db.type = UNIFORM;
			db.binding = binding->binding;
			db.buffer_size = binding->block.padded_size;
			db.name = binding->name;
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
			db.type = TEXTURE;
			db.binding = binding->binding;
			if (binding->image.dim == SpvDim::SpvDim3D)
				db.texture_is_3d = true;
			else if (binding->image.dim != SpvDim::SpvDim2D)
				DBG_WARNING("shader contains an unsupported image sampler dimension.");
			db.name = binding->name;
			bindings.push_back(db);
		}
	}

	spvReflectDestroyShaderModule(&reflected_module);
	return bindings;
}

VkShaderModule Shader::createShaderModule(const vector<uint32_t>& blob)
{
	VkShaderModuleCreateInfo create_info{ };
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = blob.size() * 4;
	create_info.pCode = blob.data();

	VkShaderModule shader_module;
	if (vkCreateShaderModule(RenderServer::getDevice(), &create_info, nullptr, &shader_module) != VK_SUCCESS)
		DBG_FAULT("vkCreateShaderModule failed");

	return shader_module;
}

bool Shader::compileShaders(const string& path, vector<uint32_t>& vert_blob, vector<uint32_t>& frag_blob)
{
	auto shader_data = Package::tryLoadFile(path);
	
	if (shader_data.empty())
	{
		DBG_ERROR("shader " + path + " not found");
		return false;
	}
	
	filesystem::path current_file_path;
	bool is_res_relative = false;
	if (path.starts_with("res://"))
	{
		current_file_path = path.substr(6);
		is_res_relative = true;		
	}
	else
		current_file_path = path;
	const string current_file_location = current_file_path.remove_filename().string();
	
	string shader_text; shader_text.resize(shader_data.size());
	memcpy(shader_text.data(), shader_data.data(), shader_data.size());
	
	Shader::fixIncludes(shader_text, current_file_location, is_res_relative);
	string vertex_text;
	string fragment_text;
	Shader::preprocess(shader_text, vertex_text, fragment_text, path);
	if (vertex_text.empty() || fragment_text.empty())
		return false;
	
	const shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	options.AddMacroDefinition("vertex", "main");
	auto result = compiler.CompileGlslToSpv(vertex_text.data(), vertex_text.size(), shaderc_glsl_vertex_shader, path.c_str(), options);
	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		DBG_ERROR("error compiling vertex shader " + path + ": " + result.GetErrorMessage());
		DBG_INFO("see the full shader code below: " + vertex_text);
		return false;
	}
	vert_blob = { result.cbegin(), result.cend() };
	
	shaderc::CompileOptions options2;
	options2.AddMacroDefinition("fragment", "main");
	result = compiler.CompileGlslToSpv(fragment_text.data(), fragment_text.size(), shaderc_glsl_fragment_shader, path.c_str(), options2);
	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		DBG_ERROR("error compiling fragment shader " + path + ": " + result.GetErrorMessage());
		DBG_INFO("see the full shader code below: " + fragment_text);
		return false;
	}
	frag_blob = { result.cbegin(), result.cend() };
	
	return true;
}

void Shader::createDescriptorSetLayout()
{
	vector<VkDescriptorSetLayoutBinding> layout_bindings;
	for (const DescriptorBinding& binding : bindings)
	{
		VkDescriptorSetLayoutBinding layout_binding{ };
		layout_binding.binding = binding.binding;
		layout_binding.descriptorType = (binding.type == UNIFORM) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layout_binding.descriptorCount = 1;
		layout_binding.pImmutableSamplers = nullptr;
		layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layout_bindings.push_back(layout_binding);
	}

	VkDescriptorSetLayoutCreateInfo set_layout_create_info{ };
	set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set_layout_create_info.bindingCount = static_cast<uint32_t>(layout_bindings.size());
	set_layout_create_info.pBindings = layout_bindings.data();

	if (vkCreateDescriptorSetLayout(RenderServer::getDevice(), &set_layout_create_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
		DBG_FAULT("vkCreateDescriptorSetLayout failed");
}

void Shader::destroyResources()
{
    RenderServer::waitIdle();
	vkDestroyPipelineLayout(RenderServer::getDevice(), pipeline_layout, nullptr);
	vkDestroyDescriptorSetLayout(RenderServer::getDevice(), descriptor_set_layout, nullptr);

	vkDestroyShaderModule(RenderServer::getDevice(), vert_module, nullptr);
	vkDestroyShaderModule(RenderServer::getDevice(), frag_module, nullptr);
}
