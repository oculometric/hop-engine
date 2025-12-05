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
	std::string name;
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
		
	// TODO: direct string shader constructor
	Shader(std::string base_path, bool is_precompiled);
	~Shader();

	inline VkPipelineLayout getPipelineLayout() { return pipeline_layout; }
	std::vector<VkPipelineShaderStageCreateInfo> getShaderStageCreateInfos();
	ShaderLayout getShaderLayout();

private:
	static std::vector<DescriptorBinding> mergeBindings(std::vector<DescriptorBinding> list_a, std::vector<DescriptorBinding> list_b);
	static std::vector<DescriptorBinding> getReflectedBindings(std::vector<char> blob);
	static bool compileFile(std::string path, std::string out_path);
	static std::vector<char> readFile(std::string path);
	static VkShaderModule createShaderModule(const std::vector<char>& blob);
};

}
