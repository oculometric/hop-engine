#include "shader.h"

#include <map>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <spirv_reflect/spirv_reflect.h>
#include <filesystem>

#include "graphics_environment.h"
#include "render_pass.h"
#include "package.h"

using namespace HopEngine;
using namespace std;

const char* Shader::compiler_path = "C:/tmp/glslc.exe";

Shader::Shader(string base_path, bool is_precompiled)
{
	string proper_path = base_path;
	if (!is_precompiled)
	{
		proper_path = Package::getTempPath() + "temp_shader_compiled";
		if (!compileShaders(base_path, proper_path))
		{
			DBG_ERROR(base_path + " shader compilation failed");
			if (!compileShaders("res://shader", proper_path))
				DBG_FAULT("failed to load default shader!");
		}
	}
	
	auto vert_blob = Package::tryLoadFile(proper_path + "_vert.spv");
	auto frag_blob = Package::tryLoadFile(proper_path + "_frag.spv");

	vert_module = createShaderModule(vert_blob);
	frag_module = createShaderModule(frag_blob);

	auto vert_bindings = getReflectedBindings(vert_blob);
	auto frag_bindings = getReflectedBindings(frag_blob);

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

	if (vkCreateDescriptorSetLayout(GraphicsEnvironment::get()->getDevice(), &set_layout_create_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
		DBG_FAULT("vkCreateDescriptorSetLayout failed");

	VkPipelineLayoutCreateInfo layout_create_info{ };
	layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_create_info.setLayoutCount = 3;
	VkDescriptorSetLayout layouts[3] =
	{
		GraphicsEnvironment::get()->getSceneDescriptorSetLayout(),
		GraphicsEnvironment::get()->getObjectDescriptorSetLayout(),
		descriptor_set_layout
		
	};
	layout_create_info.pSetLayouts = layouts;

	if (vkCreatePipelineLayout(GraphicsEnvironment::get()->getDevice(), &layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS)
		DBG_FAULT("vkCreatePipelineLayout failed");

	DBG_INFO("created shader from " + base_path);
}

Shader::~Shader()
{
	DBG_INFO("destroyed shader " + PTR(this));

	vkDestroyPipelineLayout(GraphicsEnvironment::get()->getDevice(), pipeline_layout, nullptr);
	vkDestroyDescriptorSetLayout(GraphicsEnvironment::get()->getDevice(), descriptor_set_layout, nullptr);

	vkDestroyShaderModule(GraphicsEnvironment::get()->getDevice(), vert_module, nullptr);
	vkDestroyShaderModule(GraphicsEnvironment::get()->getDevice(), frag_module, nullptr);
}

vector<VkPipelineShaderStageCreateInfo> Shader::getShaderStageCreateInfos()
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

ShaderLayout Shader::getShaderLayout()
{
	return { descriptor_set_layout, bindings };
}

vector<DescriptorBinding> Shader::mergeBindings(vector<DescriptorBinding> list_a, vector<DescriptorBinding> list_b)
{
	multimap<uint32_t, DescriptorBinding> bindings;

	for (auto item : list_a)
		bindings.insert({ item.binding, item });
	for (auto item : list_b)
		bindings.insert({ item.binding, item });

	if (bindings.empty())
		return { };
	if (bindings.size() == 1)
		return { bindings.begin()->second };

	vector<DescriptorBinding> resolved_bindings;

	uint32_t last_binding_index = 0;
	auto binding_it = bindings.begin();
	while (binding_it != bindings.end())
	{
		DescriptorBinding last_binding = binding_it->second;
		resolved_bindings.push_back(last_binding);
		binding_it++;
		if (binding_it == bindings.end())
			return resolved_bindings;
		else if (binding_it->first == last_binding.binding)
		{
			// uh oh! duplicate bindings! that's not good...
			if (binding_it->second.type == last_binding.type && binding_it->second.buffer_size == last_binding.buffer_size)
				binding_it++;
			else
			{
				DBG_ERROR("incompatible duplicate shader uniform/texture bindings found");
				binding_it++;
			}
		}
	}

	return resolved_bindings;
}

vector<DescriptorBinding> Shader::getReflectedBindings(vector<uint8_t> blob)
{
	SpvReflectShaderModule reflected_module;
	SpvReflectResult result = spvReflectCreateShaderModule(blob.size(), blob.data(), &reflected_module);
	if (result != SPV_REFLECT_RESULT_SUCCESS)
	{
		DBG_WARNING("unable to construct reflection module");
		return { };
	}
	uint32_t descriptor_sets = 0;
	spvReflectEnumerateDescriptorSets(&reflected_module, &descriptor_sets, nullptr);
	if (descriptor_sets < 3)
	{
		DBG_VERBOSE("shader module does not have a descriptor set 2");
		return { };
	}
	const SpvReflectDescriptorSet* vert_material_set = spvReflectGetDescriptorSet(&reflected_module, 2, &result);
	if (result != SPV_REFLECT_RESULT_SUCCESS)
	{
		DBG_WARNING("unable to reflect descriptor set 2");
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

bool Shader::compileFile(string path, string out_path)
{
	string compile_command = Shader::compiler_path;
#if defined(_WIN32)
	for (size_t i = 0; i < path.size(); i++)
	{
		if (path[i] == '/')
			path[i] = '\\';
	}
	for (size_t i = 0; i < path.size(); i++)
	{
		if (path[i] == '/')
			path[i] = '\\';
	}
#endif

	string command_out;
	compile_command = compile_command + ' ' + path + " -o " + out_path;
	int result = exec(compile_command.c_str(), command_out);

	if (result != 0)
	{
		DBG_WARNING("error when compiling '" + path + "':\n" + command_out);
		return false;
	}

	return true;
}

VkShaderModule Shader::createShaderModule(const vector<uint8_t>& blob)
{
	VkShaderModuleCreateInfo create_info{ };
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = blob.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(blob.data());

	VkShaderModule shader_module;
	if (vkCreateShaderModule(GraphicsEnvironment::get()->getDevice(), &create_info, nullptr, &shader_module) != VK_SUCCESS)
		DBG_FAULT("vkCreateShaderModule failed");

	return shader_module;
}

void Shader::fixIncludes(vector<uint8_t>& source_code, string path_prefix)
{
	string source_code_text(source_code.size(), ' ');
	memcpy(source_code_text.data(), source_code.data(), source_code.size());

	string include_search = "#include \"";
	size_t offset = source_code_text.find(include_search, 0);
	while (offset != string::npos)
	{
		size_t start = offset + include_search.size();
		size_t end = source_code_text.find('\"', start);
		string path = source_code_text.substr(start, end - start);
		if (path.find(' ') != string::npos)
		{
			DBG_ERROR("malformed include found!");
			source_code.resize(source_code_text.size());
			memcpy(source_code.data(), source_code_text.data(), source_code_text.size());
			return;
		}
		source_code_text.erase(offset, (end - offset) + 1);
		auto include_data = Package::tryLoadFile(path_prefix + path);
		string include_string(include_data.size(), ' ');
		memcpy(include_string.data(), include_data.data(), include_data.size());
		source_code_text.insert(source_code_text.begin() + offset, include_data.begin(), include_data.end());

		offset = source_code_text.find(include_search, offset);
	}

	source_code.resize(source_code_text.size());
	memcpy(source_code.data(), source_code_text.data(), source_code_text.size());
}

bool Shader::compileShaders(string path, string out_path)
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

	filesystem::path _path = path;
	string prefix = _path.remove_filename().string();
	Shader::fixIncludes(vert_data, prefix);
	Shader::fixIncludes(frag_data, prefix);

	string input_path = Package::getTempPath() + "temp_shader";
	Package::tryWriteFile(input_path + ".vert", vert_data);
	Package::tryWriteFile(input_path + ".frag", frag_data);

	bool result = Shader::compileFile(input_path + ".vert", out_path + "_vert.spv");
	result &= Shader::compileFile(input_path + ".frag", out_path + "_frag.spv");

	return result;
}
