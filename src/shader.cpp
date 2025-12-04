#include "shader.h"

#include <fstream>

#include "graphics_environment.h"
#include "render_pass.h"

using namespace HopEngine;
using namespace std;

Shader::Shader(string base_path)
{
	auto vert_blob = Shader::readFile(base_path + "_vert.spv");
	auto frag_blob = Shader::readFile(base_path + "_frag.spv");

	vert_module = createShaderModule(vert_blob);
	frag_module = createShaderModule(frag_blob);

	// TODO: we should read the bindings and buffer size for set 2 from the shader reflection data
	VkDescriptorSetLayoutBinding uniform_layout_binding{ };
	uniform_layout_binding.binding = 0;
	uniform_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniform_layout_binding.descriptorCount = 1;
	uniform_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo set_layout_create_info{ };
	set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set_layout_create_info.bindingCount = 1;
	set_layout_create_info.pBindings = &uniform_layout_binding;

	uniform_buffer_size = 16;

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

pair<VkDescriptorSetLayout, VkDeviceSize> Shader::getMaterialUniformConfig()
{
	return { descriptor_set_layout, uniform_buffer_size };
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
