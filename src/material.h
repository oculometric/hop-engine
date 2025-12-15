#pragma once

#include <map>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "common.h"
#include "shader.h"
#include "render_pass.h"
#include "pipeline.h"

namespace HopEngine
{

class Material
{
private:
	Ref<Shader> shader;
	Ref<Pipeline> pipeline;
	Ref<UniformBlock> uniforms;
	Ref<RenderPass> render_pass;
	std::map<std::string, uint32_t> texture_name_to_binding;
	std::map<std::string, UniformVariable> variable_name_to_binding;

public:
	DELETE_CONSTRUCTORS(Material);

	Material(Ref<Shader> shader, PipelineBuilder config = PipelineBuilder(), Ref<RenderPass> render_pass = nullptr);
	~Material();

	VkPipeline getPipeline() const;
	VkPipelineLayout getPipelineLayout() const;
	void pushToDescriptorSet(size_t index);
	VkDescriptorSet getDescriptorSet(size_t index) const;
	Ref<Shader> getShader() const;
	Ref<RenderPass> getRenderPass() const;
	
	void setTexture(uint32_t binding, Ref<Texture> texture);
	void setSampler(uint32_t binding, Ref<Sampler> sampler);
	void setTexture(std::string name, Ref<Texture> texture);
	void setSampler(std::string name, Ref<Sampler> sampler);

	inline void setFloatUniform(std::string name, float value) { setUniform(name, &value, sizeof(value)); }
	inline void setVec2Uniform(std::string name, glm::vec2 value) { setUniform(name, &value, sizeof(value)); }
	inline void setVec3Uniform(std::string name, glm::vec3 value) { setUniform(name, &value, sizeof(value)); }
	inline void setVec4Uniform(std::string name, glm::vec4 value) { setUniform(name, &value, sizeof(value)); }

	inline void setIntUniform(std::string name, int value) { setUniform(name, &value, sizeof(value)); }
	inline void setIvec2Uniform(std::string name, glm::ivec2 value) { setUniform(name, &value, sizeof(value)); }
	inline void setIvec3Uniform(std::string name, glm::ivec3 value) { setUniform(name, &value, sizeof(value)); }
	inline void setIvec4Uniform(std::string name, glm::ivec4 value) { setUniform(name, &value, sizeof(value)); }

	inline void setUintUniform(std::string name, glm::uint value) { setUniform(name, &value, sizeof(value)); }

	inline void setBoolUniform(std::string name, bool value) { VkBool32 expanded = value; setUniform(name, &expanded, sizeof(VkBool32)); }

	inline void setMat2Uniform(std::string name, glm::mat2 value) { setUniform(name, &value, sizeof(value)); }
	inline void setMat3Uniform(std::string name, glm::mat3 value) { setUniform(name, &value, sizeof(value)); }
	inline void setMat4Uniform(std::string name, glm::mat4 value) { setUniform(name, &value, sizeof(value)); }

	void setUniform(std::string name, void* data, size_t size);

	static Ref<Material> deserialise(std::string name);

private:
};

}
