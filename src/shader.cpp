#include "shader.h"

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

#include "graphics_environment.h"
#include "render_pass.h"

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
		proper_path = "_TEMP_SHADER";

		bool result = Shader::compileFile(base_path + ".vert", proper_path + "_vert.spv");
		if (!result)
			throw runtime_error("shader compilation failed");
		result = Shader::compileFile(base_path + ".frag", proper_path + "_frag.spv");
		if (!result)
			throw runtime_error("shader compilation failed");
	}
	
	auto vert_blob = Shader::readFile(proper_path + "_vert.spv");
	auto frag_blob = Shader::readFile(proper_path + "_frag.spv");

	vert_module = createShaderModule(vert_blob);
	frag_module = createShaderModule(frag_blob);

	// TODO: we should read the bindings and buffer size for set 2 from the shader reflection data
	bindings =
	{
		{ 0, UNIFORM, 16 },
		{ 1, TEXTURE }
	};

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

bool Shader::compileFile(string path, string out_path)
{
	// TODO: compile the shaders
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

vector<char> Shader::readFile(string path)
{
	ifstream file(path, ios::ate | ios::binary);
	if (!file.is_open())
		return { };

	size_t size = (size_t)file.tellg();
	vector<char> buffer(size);
	file.seekg(0);
	file.read(buffer.data(), size);
	file.close();

	return buffer;
}

VkShaderModule Shader::createShaderModule(const vector<char>& blob)
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
