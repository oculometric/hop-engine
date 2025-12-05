#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "common.h"

namespace HopEngine
{

enum DescriptorBindingType
{
	UNIFORM,
	TEXTURE
};

struct DescriptorBinding
{
	uint32_t binding;
	DescriptorBindingType type;
	VkDeviceSize buffer_size;
};

struct ShaderLayout
{
	VkDescriptorSetLayout layout = VK_NULL_HANDLE;
	std::vector<DescriptorBinding> bindings;
};

class Shader
{
private:
	VkShaderModule vert_module = VK_NULL_HANDLE;
	VkShaderModule frag_module = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
	std::vector<DescriptorBinding> bindings;

public:
	DELETE_CONSTRUCTORS(Shader);
		
	// TODO: direct string shader constructor, and runtime compilation
	Shader(std::string base_path, bool is_precompiled);
	~Shader();

	inline VkPipelineLayout getPipelineLayout() { return pipeline_layout; }
	std::vector<VkPipelineShaderStageCreateInfo> getShaderStageCreateInfos();
	ShaderLayout getShaderLayout();

private:
	static bool compileFile(std::string path, std::string out_path);
	static std::vector<char> readFile(std::string path);
	static VkShaderModule createShaderModule(const std::vector<char>& blob);
};

}
