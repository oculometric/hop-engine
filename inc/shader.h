#pragma once

#include <string>
#include <vector>

#include "common.h"
#include "vulkan_typedefs.h"

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

class Shader : public Destructible
{
public:
	static const char* compiler_path;

private:
	std::string origin;
	VkShaderModule vert_module = VK_NULL_HANDLE;
	VkShaderModule frag_module = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
	std::vector<DescriptorBinding> bindings;

public:
	DELETE_CONSTRUCTORS(Shader);
	Shader(const std::string& base_path);
	~Shader() override;
	
	std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }
	VkPipelineLayout getPipelineLayout() const { return pipeline_layout; }
	ShaderLayout getShaderLayout() const;
	void bind(Ref<DrawCommandBuffer> command_buffer);
	
	std::vector<VkPipelineShaderStageCreateInfo> getShaderStageCreateInfos() const;
	bool reloadShader(); // TODO: shader reload

private:
	static std::vector<DescriptorBinding> mergeBindings(const std::vector<DescriptorBinding>& list_a, const std::vector<DescriptorBinding>& list_b);
	static std::vector<DescriptorBinding> getReflectedBindings(const std::vector<uint32_t>& blob);
	static VkShaderModule createShaderModule(const std::vector<uint32_t>& blob);
	static void fixIncludes(std::vector<uint8_t>& source_code, const std::string& path_prefix, bool res_relative);
	static bool compileShaders(const std::string& path, std::vector<uint32_t>& vert_blob, std::vector<uint32_t>& frag_blob);
};

}
