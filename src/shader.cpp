#include "shader.h"

#include <map>
#include <vector>
#include <string>
#include <vulkan/vulkan.hpp>
#include <shaderc/shaderc.hpp>
#include <spirv_reflect/spirv_reflect.h>
#include <filesystem>

#include "command_buffer.h"
#include "graphics_environment.h"
#include "package.h"

using namespace HopEngine;
using namespace std;

Shader::Shader(const string& base_path)
{
	origin = base_path;
	vector<uint32_t> vert_blob;
	vector<uint32_t> frag_blob;
	if (!compileShaders(base_path, vert_blob, frag_blob))
	{
		DBG_ERROR(base_path + " shader compilation failed");
		if (!compileShaders("res://engine/shaders/default_shader", vert_blob, frag_blob))
			DBG_FAULT("failed to load default shader!");
	}

	vert_module = createShaderModule(vert_blob);
	frag_module = createShaderModule(frag_blob);

	const auto vert_bindings = getReflectedBindings(vert_blob);
	const auto frag_bindings = getReflectedBindings(frag_blob);

	bindings = mergeBindings(vert_bindings, frag_bindings);

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

	VkPipelineLayoutCreateInfo layout_create_info{ };
	layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_create_info.setLayoutCount = 3;
	const VkDescriptorSetLayout layouts[3] =
	{
		RenderServer::getSceneDescriptorSetLayout(),
		RenderServer::getObjectDescriptorSetLayout(),
		descriptor_set_layout
		
	};
	layout_create_info.pSetLayouts = layouts;

	if (vkCreatePipelineLayout(RenderServer::getDevice(), &layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS)
		DBG_FAULT("vkCreatePipelineLayout failed");

	DBG_VERBOSE("created shader from " + base_path);
}

Shader::~Shader()
{
	DBG_VERBOSE("destroyed shader '" + getOrigin() + '\'');

	vkDestroyPipelineLayout(RenderServer::getDevice(), pipeline_layout, nullptr);
	vkDestroyDescriptorSetLayout(RenderServer::getDevice(), descriptor_set_layout, nullptr);

	vkDestroyShaderModule(RenderServer::getDevice(), vert_module, nullptr);
	vkDestroyShaderModule(RenderServer::getDevice(), frag_module, nullptr);
}

ShaderLayout Shader::getShaderLayout() const
{
	return { descriptor_set_layout, bindings };
}

void Shader::bind(Ref<DrawCommandBuffer> command_buffer)
{
	command_buffer->bindPipelineLayoutInternal(pipeline_layout);
}

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

bool Shader::reloadShader()
{
	RenderServer::waitIdle();

	// string proper_path = origin;
	// if (!precompiled)
	// {
	// 	proper_path = Package::getTempPath() + "temp_shader_compiled";
	// 	if (!compileShaders(origin, proper_path))
	// 	{
	// 		DBG_ERROR(origin + " shader compilation failed");
	// 		return false;
	// 	}
	// }
	//
	// auto vert_blob = Package::tryLoadFile(proper_path + "_vert.spv");
	// auto frag_blob = Package::tryLoadFile(proper_path + "_frag.spv");
	//
	// vkDestroyShaderModule(RenderServer::getDevice(), vert_module, nullptr);
	// vert_module = createShaderModule(vert_blob);
	// vkDestroyShaderModule(RenderServer::getDevice(), frag_module, nullptr);
	// frag_module = createShaderModule(frag_blob);
	//
	// auto vert_bindings = getReflectedBindings(vert_blob);
	// auto frag_bindings = getReflectedBindings(frag_blob);
	//
	// bindings = mergeBindings(vert_bindings, frag_bindings);
	//
	// vector<VkDescriptorSetLayoutBinding> layout_bindings;
	// for (const DescriptorBinding& binding : bindings)
	// {
	// 	VkDescriptorSetLayoutBinding layout_binding{ };
	// 	layout_binding.binding = binding.binding;
	// 	layout_binding.descriptorType = (binding.type == UNIFORM) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// 	layout_binding.descriptorCount = 1;
	// 	layout_binding.pImmutableSamplers = nullptr;
	// 	layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	// 	layout_bindings.push_back(layout_binding);
	// }
	//
	// VkDescriptorSetLayoutCreateInfo set_layout_create_info{ };
	// set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	// set_layout_create_info.bindingCount = static_cast<uint32_t>(layout_bindings.size());
	// set_layout_create_info.pBindings = layout_bindings.data();
	//
	// if (vkCreateDescriptorSetLayout(RenderServer::getDevice(), &set_layout_create_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
	// 	DBG_FAULT("vkCreateDescriptorSetLayout failed");
	//
	// VkPipelineLayoutCreateInfo layout_create_info{ };
	// layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	// layout_create_info.setLayoutCount = 3;
	// VkDescriptorSetLayout layouts[3] =
	// {
	// 	RenderServer::getSceneDescriptorSetLayout(),
	// 	RenderServer::getObjectDescriptorSetLayout(),
	// 	descriptor_set_layout
	// };
	// layout_create_info.pSetLayouts = layouts;
	//
	// if (vkCreatePipelineLayout(RenderServer::getDevice(), &layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS)
	// 	DBG_FAULT("vkCreatePipelineLayout failed");
	//
	// DBG_VERBOSE("recompiled shader from " + origin);
	return true;
}

vector<DescriptorBinding> Shader::mergeBindings(const vector<DescriptorBinding>& list_a, const vector<DescriptorBinding>& list_b)
{
	multimap<uint32_t, DescriptorBinding> bindings;

	for (const auto& item : list_a)
		bindings.insert({ item.binding, item });
	for (const auto& item : list_b)
		bindings.insert({ item.binding, item });

	if (bindings.empty())
		return { };
	if (bindings.size() == 1)
		return { bindings.begin()->second };

	vector<DescriptorBinding> resolved_bindings;

	auto binding_it = bindings.begin();
	while (binding_it != bindings.end())
	{
		DescriptorBinding last_binding = binding_it->second;
		resolved_bindings.push_back(last_binding);
		++binding_it;
		if (binding_it == bindings.end())
			return resolved_bindings;
		if (binding_it->first == last_binding.binding)
		{
			// uh oh! duplicate bindings! that's not good...
			if (binding_it->second.type == last_binding.type && binding_it->second.buffer_size == last_binding.buffer_size)
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

vector<DescriptorBinding> Shader::getReflectedBindings(const vector<uint32_t>& blob)
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

void Shader::fixIncludes(vector<uint8_t>& source_code, const string& path_prefix, const bool res_relative)
{
	string source_code_text(source_code.size(), ' ');
	memcpy(source_code_text.data(), source_code.data(), source_code.size());

	const string include_search = "#include \"";
	size_t offset = source_code_text.find(include_search, 0);
	while (offset != string::npos)
	{
		const size_t start = offset + include_search.size();
		const size_t end = source_code_text.find('\"', start);
		string path = source_code_text.substr(start, end - start);
		if (path.find(' ') != string::npos)
		{
			DBG_ERROR("malformed include found!");
			source_code.resize(source_code_text.size());
			memcpy(source_code.data(), source_code_text.data(), source_code_text.size());
			return;
		}
		source_code_text.erase(offset, (end - offset) + 1);
		string target_path = path_prefix + path;
		filesystem::path fixed_path = target_path;
		auto lex = fixed_path.lexically_normal();
		string real_path = lex.string();
		for (char& value : real_path) 
			if (value == '\\')
				value = '/';
		auto include_data = Package::tryLoadFile(res_relative ? ("res://" + real_path) : real_path);
		string include_string(include_data.size(), ' ');
		memcpy(include_string.data(), include_data.data(), include_data.size());
		source_code_text.insert(source_code_text.begin() + static_cast<long long>(offset), include_data.begin(), include_data.end());

		offset = source_code_text.find(include_search, offset);
	}

	source_code.resize(source_code_text.size());
	memcpy(source_code.data(), source_code_text.data(), source_code_text.size());
}

bool Shader::compileShaders(const string& path, vector<uint32_t>& vert_blob, vector<uint32_t>& frag_blob)
{
	auto vert_data = Package::tryLoadFile(path + ".vert");
	auto frag_data = Package::tryLoadFile(path + ".frag");

	if (vert_data.empty())
	{
		DBG_WARNING("shader " + path + ".vert not found");
		return false;
	}
	if (frag_data.empty())
	{
		DBG_WARNING("shader " + path + ".frag not found");
		return false;
	}
	
	filesystem::path _path;
	bool is_res_relative = false;
	if (path.starts_with("res://"))
	{
		_path = path.substr(6);
		is_res_relative = true;		
	}
	else
		_path = path;
	const string prefix = _path.remove_filename().string();
	Shader::fixIncludes(vert_data, prefix, is_res_relative);
	Shader::fixIncludes(frag_data, prefix, is_res_relative);
	
	const shaderc::Compiler compiler;
	const shaderc::CompileOptions options;
	const auto vert_prep = compiler.CompileGlslToSpv(reinterpret_cast<char*>(vert_data.data()), vert_data.size(), shaderc_glsl_vertex_shader, path.c_str(), options);
	const auto frag_prep = compiler.CompileGlslToSpv(reinterpret_cast<char*>(frag_data.data()), frag_data.size(), shaderc_glsl_fragment_shader, path.c_str(), options);	
	
	if (vert_prep.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		DBG_ERROR("error compiling " + path + ".vert: " + vert_prep.GetErrorMessage());
		return false;
	}
	if (frag_prep.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		DBG_ERROR("error compiling " + path + ".frag: " + frag_prep.GetErrorMessage());
		return false;
	}
	
	vert_blob = { vert_prep.cbegin(), vert_prep.cend() };
	frag_blob = { frag_prep.cbegin(), frag_prep.cend() };

	return true;
}
