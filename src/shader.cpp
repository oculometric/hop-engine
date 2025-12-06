#include "shader.h"

#include <map>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <array>
#ifndef _WIN32
#include <unistd.h>
#else
#define popen _popen
#define pclose _pclose
#endif
#include <spirv_reflect/spirv_reflect.h>
#include <filesystem>

#include "graphics_environment.h"
#include "render_pass.h"
#include "package.h"

using namespace HopEngine;
using namespace std;

inline int exec(std::string command, std::string& output)
{
	const size_t buffer_size = 512;
	std::array<char, buffer_size> buffer;

	auto pipe = popen((command + " 2>&1").c_str(), "r");
	if (!pipe)
	{
		output = "popen failed.";
		return -1;
	}

	output = "";
	size_t count;
	do {
		if ((count = fread(buffer.data(), 1, buffer_size, pipe)) > 0)
			output.insert(output.end(), std::begin(buffer), std::next(std::begin(buffer), count));
	} while (count > 0);

	return pclose(pipe);
}

Shader::Shader(string base_path, bool is_precompiled)
{
	string proper_path = base_path;
	if (!is_precompiled)
	{
		auto vert_data = Package::tryLoadFile(base_path + ".vert");
		auto frag_data = Package::tryLoadFile(base_path + ".frag");
		filesystem::path path = base_path;
		string prefix = path.remove_filename().string();
		Shader::fixIncludes(vert_data, prefix);
		Shader::fixIncludes(frag_data, prefix);

		string input_path = Package::getTempPath() + "temp_shader";
		Package::tryWriteFile(input_path + ".vert", vert_data);
		Package::tryWriteFile(input_path + ".frag", frag_data);

		proper_path = Package::getTempPath() + "temp_shader_compiled";

		bool result = Shader::compileFile(input_path + ".vert", proper_path + "_vert.spv");
		if (!result)
			throw runtime_error("shader compilation failed");
		result = Shader::compileFile(input_path + ".frag", proper_path + "_frag.spv");
		if (!result)
			throw runtime_error("shader compilation failed");
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
		throw runtime_error("vkCreateDescriptorSetLayout failed");

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
		throw runtime_error("vkCreatePipelineLayout failed");
}

Shader::~Shader()
{
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
				throw runtime_error("incompatible duplicate shader uniform/texture bindings found");
		}
	}

	return resolved_bindings;
}

vector<DescriptorBinding> Shader::getReflectedBindings(vector<uint8_t> blob)
{
	SpvReflectShaderModule reflected_module;
	SpvReflectResult result = spvReflectCreateShaderModule(blob.size(), blob.data(), &reflected_module);
	if (result != SPV_REFLECT_RESULT_SUCCESS)
		return {};
	const SpvReflectDescriptorSet* vert_material_set = spvReflectGetDescriptorSet(&reflected_module, 2, &result);
	if (result != SPV_REFLECT_RESULT_SUCCESS)
	{
		spvReflectDestroyShaderModule(&reflected_module);
		return {};
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
				db.variables.push_back({ member.name, member.padded_size, member.offset });
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
	string compile_command = "glslc";
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
		cout << "error when compiling " << path << endl << command_out;
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
		throw runtime_error("vkCreateShaderModule failed");

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
			throw runtime_error("malformed include!");
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
