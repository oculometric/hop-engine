#pragma once

#include <map>
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "common.h"
#include "shader.h"

namespace HopEngine
{

// shader uniform layout:
// set 0 -> scene uniforms (time, world to view, view to clip): 1 per frame-in-flight (managed by the environment)
// set 1 -> object uniforms (object id, object to world): 1 per object, per frame-in-flight (managed by the object)
// set 2 -> material uniforms (these are customisable): 1 per material, per frame-in-flight (managed by the material)

// the shader tells us about the layout, but the first 2 sets will NOT be read from the shader
// we create the layouts for the first two sets when the environment loads, since they are the same for all shaders
class Shader;
class Buffer;
class Pipeline;
class UniformBlock;
class Texture;
class Sampler;

class Material
{
private:
	Ref<Shader> shader = nullptr;
	Ref<Pipeline> pipeline = nullptr;
	Ref<UniformBlock> uniforms = nullptr;
	std::map<std::string, uint32_t> texture_name_to_binding;
	std::map<std::string, UniformVariable> variable_name_to_binding;

public:
	DELETE_CONSTRUCTORS(Material);

	Material(Ref<Shader> shader, VkCullModeFlags culling_mode, VkPolygonMode polygon_mode);
	~Material();

	VkPipeline getPipeline();
	VkPipelineLayout getPipelineLayout();
	void pushToDescriptorSet(size_t index);
	VkDescriptorSet getDescriptorSet(size_t index);

	void setTexture(size_t index, Ref<Texture> texture);
	void setSampler(size_t index, Ref<Sampler> sampler);
	void setTexture(std::string name, Ref<Texture> texture);
	void setSampler(std::string name, Ref<Sampler> sampler);

	inline void setFloatUniform(std::string name, float value) { setUniformVariable(name, &value, sizeof(value)); }
	void setVec2Uniform(std::string name, glm::vec2 value) { setUniformVariable(name, &value, sizeof(value)); }
	void setVec3Uniform(std::string name, glm::vec3 value) { setUniformVariable(name, &value, sizeof(value)); }
	void setVec4Uniform(std::string name, glm::vec4 value) { setUniformVariable(name, &value, sizeof(value)); }

	void setIntUniform(std::string name, int value) { setUniformVariable(name, &value, sizeof(value)); }
	void setIvec2Uniform(std::string name, glm::ivec2 value) { setUniformVariable(name, &value, sizeof(value)); }
	void setIvec3Uniform(std::string name, glm::ivec3 value) { setUniformVariable(name, &value, sizeof(value)); }
	void setIvec4Uniform(std::string name, glm::ivec4 value) { setUniformVariable(name, &value, sizeof(value)); }

	void setUintUniform(std::string name, glm::uint value) { setUniformVariable(name, &value, sizeof(value)); }

	void setBoolUniform(std::string name, bool value) { setUniformVariable(name, &value, sizeof(value)); }

	void setMat2Uniform(std::string name, glm::mat2 value) { setUniformVariable(name, &value, sizeof(value)); }
	void setMat3Uniform(std::string name, glm::mat3 value) { setUniformVariable(name, &value, sizeof(value)); }
	void setMat4Uniform(std::string name, glm::mat4 value) { setUniformVariable(name, &value, sizeof(value)); }

private:
	void setUniformVariable(std::string name, void* data, size_t size);
};

}
