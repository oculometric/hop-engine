#pragma once

#include <map>
#include <glm/glm.hpp>

#include "common.h"
#include "render_pass.h"
#include "pipeline.h"
#include "vulkan_typedefs.h"

namespace HopEngine
{

/**
 * @brief describes a 3D scene light. see the \code LightComponent\endcode class.
 */
struct LightParams final
{
    glm::vec4 position = { 2, 0, 2, 0 };
    glm::vec4 direction = { -1, 0, -1, 0 };
    glm::vec4 colour = { 1, 0, 0, 0 };
    float spot_angle = 0.0f;
    int light_type = 0;
    bool enabled = false;
    float padding;
};

/**
 * @brief structure which mirrors the standard object uniform
 * buffer (i.e. descriptor set 1).
 */
struct ObjectUniforms final
{
    glm::mat4 model_to_world;
    int id;
};

/**
 * @brief structure which mirrors the standard scene uniform
 * buffer (i.e. descriptor set 0).
 */
struct SceneUniforms final
{
    glm::mat4 world_to_view;
    glm::mat4 view_to_clip;
    glm::mat4 clip_to_view;
    glm::mat4 view_to_world;
    glm::ivec2 viewport_size = { 0, 0 };
    glm::vec2 padding = { 0, 0 };
    glm::vec3 eye_position = { 0, 0, 0 };
    float time = 0;
    glm::vec2 near_far = { 0, 0 };
    glm::vec2 padding2 = { 0, 0 };
    LightParams lights[8];
    glm::vec4 ambient_light = { 0, 0.05f, 0.05f, 0 };
};

class Shader final : public Destructible
{
public:
	static const char* compiler_path;
	
	enum DescriptorBindingType
	{
		UNIFORM,
		TEXTURE
	};

	struct UniformVariable final
	{
		std::string name;
		size_t size = 0;
		size_t offset = 0;
	};

	struct DescriptorBinding final
	{
		uint32_t binding = 0;
		DescriptorBindingType type = UNIFORM;
		VkDeviceSize buffer_size = 0;
		std::string name;
		std::vector<UniformVariable> variables;
		bool texture_is_3d = false;
	};

	struct Layout final
	{
		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		std::vector<DescriptorBinding> bindings;
		uint32_t set_index = 2;
	};

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
	Layout getShaderLayout() const { return { descriptor_set_layout, bindings }; }
	void bind(WeakRef<DrawCommandBuffer> command_buffer);
	
	std::vector<VkPipelineShaderStageCreateInfo> getShaderStageCreateInfos() const;
	bool reloadShader(); // TODO: shader reload

private:
	static std::vector<DescriptorBinding> mergeBindings(const std::vector<DescriptorBinding>& list_a, const std::vector<DescriptorBinding>& list_b);
	static std::vector<DescriptorBinding> getReflectedBindings(const std::vector<uint32_t>& blob);
	static VkShaderModule createShaderModule(const std::vector<uint32_t>& blob);
	static void fixIncludes(std::string& source_code, const std::string& path_prefix, bool res_relative);
	static void preprocess(const std::string& source_code, std::string& vertex_shader_code, std::string& fragment_shader_code, const std::string& path);
	static std::string preprocessVertex(const std::string& common_code, const std::string& path);
	static std::string preprocessFragment(const std::string& common_code, const std::string& path);
	static void removeFunction(std::string& code, const std::string& signature);
	static void destroyAllPragmas(std::string& code);
	static bool compileShaders(const std::string& path, std::vector<uint32_t>& vert_blob, std::vector<uint32_t>& frag_blob);
	
	void createDescriptorSetLayout();
	void destroyResources();
};

class Material final : public Destructible
{
private:
	std::string origin;
	Ref<Shader> shader;
	Ref<Pipeline> pipeline;
	Ref<Pipeline> debug_pipeline;
	Ref<UniformBlock> uniforms;
	Ref<RenderPass> render_pass;
	std::map<std::string, uint32_t> texture_name_to_binding;
	std::map<std::string, Shader::UniformVariable> variable_name_to_binding;

public:
	DELETE_CONSTRUCTORS(Material);
	Material(Ref<Shader> _shader, const Pipeline::Builder& config = Pipeline::Builder(), WeakRef<RenderPass> _render_pass = nullptr);
	~Material() override;
	
	std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }
	Ref<Shader> getShader() const;
	Ref<RenderPass> getRenderPass() const;
	Ref<Material> duplicate() const;
	
	void bind(WeakRef<DrawCommandBuffer> command_buffer, bool wireframe_allowed = true);
	
	void setTexture(uint32_t binding, WeakRef<Texture> texture);
	void setSampler(uint32_t binding, WeakRef<Sampler> sampler);
	void setTexture(const std::string& name, WeakRef<Texture> texture);
	void setSampler(const std::string& name, WeakRef<Sampler> sampler);

	void setFloatUniform(const std::string& name, float value) { setUniform(name, &value, sizeof(value)); }
	void setVec2Uniform(const std::string& name, glm::vec2 value) { setUniform(name, &value, sizeof(value)); }
	void setVec3Uniform(const std::string& name, glm::vec3 value) { setUniform(name, &value, sizeof(value)); }
	void setVec4Uniform(const std::string& name, glm::vec4 value) { setUniform(name, &value, sizeof(value)); }

	void setIntUniform(const std::string& name, int value) { setUniform(name, &value, sizeof(value)); }
	void setIvec2Uniform(const std::string& name, glm::ivec2 value) { setUniform(name, &value, sizeof(value)); }
	void setIvec3Uniform(const std::string& name, glm::ivec3 value) { setUniform(name, &value, sizeof(value)); }
	void setIvec4Uniform(const std::string& name, glm::ivec4 value) { setUniform(name, &value, sizeof(value)); }

	void setUintUniform(const std::string& name, glm::uint value) { setUniform(name, &value, sizeof(value)); }
	
	void setBoolUniform(const std::string& name, bool value) { setUniform(name, &value, sizeof(uint32_t)); }

	void setMat2Uniform(const std::string& name, glm::mat2 value) { setUniform(name, &value, sizeof(value)); }
	void setMat3Uniform(const std::string& name, glm::mat3 value) { setUniform(name, &value, sizeof(value)); }
	void setMat4Uniform(const std::string& name, glm::mat4 value) { setUniform(name, &value, sizeof(value)); }

	void setUniform(const std::string& name, const void* data, size_t size);

	static Ref<Material> deserialise(const std::string& name);
	
	void drawImGuiDebug();
};

}
