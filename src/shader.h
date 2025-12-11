#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "common.h"

namespace HopEngine
{

enum DescriptorBindingType
{
	UNIFORM,
	TEXTURE
};

struct UniformVariable
{
	std::string name;
	size_t size = 0;
	size_t offset = 0;
};

struct DescriptorBinding
{
	uint32_t binding = 0;
	DescriptorBindingType type = UNIFORM;
	VkDeviceSize buffer_size = 0;
	std::string name;
	std::vector<UniformVariable> variables;
};

struct ShaderLayout
{
	VkDescriptorSetLayout layout = VK_NULL_HANDLE;
	std::vector<DescriptorBinding> bindings;
};

class Shader
{
public:
	static const char* compiler_path;

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
	static std::vector<DescriptorBinding> getReflectedBindings(std::vector<uint8_t> blob);
	static bool compileFile(std::string path, std::string out_path);
	static VkShaderModule createShaderModule(const std::vector<uint8_t>& blob);
	static void fixIncludes(std::vector<uint8_t>& source_code, std::string path_prefix);
	static bool compileShaders(std::string path, std::string out_path);
};

}
