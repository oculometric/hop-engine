#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "common.h"

namespace HopEngine
{

class Shader
{
private:
	VkShaderModule vert_module = VK_NULL_HANDLE;
	VkShaderModule frag_module = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
	VkDeviceSize uniform_buffer_size;

public:
	DELETE_CONSTRUCTORS(Shader);
		
	Shader(std::string base_path);
	~Shader();

	inline VkPipelineLayout getPipelineLayout() { return pipeline_layout; }
	std::vector<VkPipelineShaderStageCreateInfo> getShaderStageCreateInfos();
	std::pair<VkDescriptorSetLayout, VkDeviceSize> getMaterialUniformConfig();

private:
	static std::vector<char> readFile(std::string path);
	static VkShaderModule createShaderModule(const std::vector<char>& blob);
};

}
