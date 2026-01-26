#pragma once

#include <map>
#include <glm/glm.hpp>

#include "common.h"
#include "vulkan_typedefs.h"
#include "shader.h"
#include "render_pass.h"
#include "pipeline.h"

namespace HopEngine
{

class Material : public Destructible
{
private:
	Ref<Shader> shader;
	Ref<Pipeline> pipeline;
	Ref<Pipeline> debug_pipeline;
	Ref<UniformBlock> uniforms;
	Ref<RenderPass> render_pass;
	std::map<std::string, uint32_t> texture_name_to_binding;
	std::map<std::string, UniformVariable> variable_name_to_binding;
	std::string origin;

public:
	DELETE_CONSTRUCTORS(Material);

	VkPipeline getPipeline() const;
	VkPipeline getDebugPipeline() const;
	VkPipelineLayout getPipelineLayout() const;
	void pushToDescriptorSet(size_t index);
	VkDescriptorSet getDescriptorSet(size_t index) const;
	Ref<Shader> getShader() const;
	Ref<RenderPass> getRenderPass() const;
	Ref<Material> duplicate() const;
	std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }
	
	void setTexture(uint32_t binding, Ref<Texture> texture, bool use_stencil = false);
	void setSampler(uint32_t binding, Ref<Sampler> sampler);
	void setTexture(std::string name, Ref<Texture> texture, bool use_stencil = false);
	void setSampler(std::string name, Ref<Sampler> sampler);

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

	void setUniform(std::string name, void* data, size_t size);

	static Ref<Material> deserialise(const std::string& name);

	void drawImGuiDebug();

	Material(Ref<Shader> shader, PipelineBuilder config = PipelineBuilder(), Ref<RenderPass> render_pass = nullptr);
	~Material() override;
};

}
