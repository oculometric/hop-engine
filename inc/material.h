#pragma once

#include <map>
#include <glm/glm.hpp>

#include "common.h"
#include "shader.h"
#include "render_pass.h"
#include "pipeline.h"

namespace HopEngine
{

class Material : public Destructible
{
private:
	std::string origin;
	Ref<Shader> shader;
	Ref<Pipeline> pipeline;
	Ref<Pipeline> debug_pipeline;
	Ref<UniformBlock> uniforms;
	Ref<RenderPass> render_pass;
	std::map<std::string, uint32_t> texture_name_to_binding;
	std::map<std::string, UniformVariable> variable_name_to_binding;

public:
	DELETE_CONSTRUCTORS(Material);
	Material(const Ref<Shader>& _shader, const PipelineBuilder& config = PipelineBuilder(), const Ref<RenderPass>& _render_pass = nullptr);
	~Material() override;
	
	std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }
	Ref<Shader> getShader() const;
	Ref<RenderPass> getRenderPass() const;
	void pushToDescriptorSet(size_t index);
	Ref<Material> duplicate() const;
	
	void bind(Ref<DrawCommandBuffer> command_buffer, bool wireframe_allowed = true);
	
	void setTexture(uint32_t binding, const Ref<Texture>& texture, bool use_stencil = false);
	void setSampler(uint32_t binding, const Ref<Sampler>& sampler);
	void setTexture(const std::string& name, const Ref<Texture>& texture, bool use_stencil = false);
	void setSampler(const std::string& name, const Ref<Sampler>& sampler);

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
