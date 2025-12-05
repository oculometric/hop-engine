#pragma once

#include <vulkan/vulkan.h>
#include <glm/vec3.hpp>

#include "common.h"

namespace HopEngine
{

class Material;
class Mesh;
class UniformBlock;

class Object
{
public:
	glm::vec3 position = { 0, 0, 0 };
	glm::vec3 rotation = { 0, 0, 0 };
	glm::vec3 scale = { 1, 1, 1 };
	Ref<Mesh> mesh = nullptr;
	Ref<Material> material = nullptr;

private:
	Ref<UniformBlock> uniforms = nullptr;

public:
	DELETE_CONSTRUCTORS(Object);

	Object(Ref<Mesh> mesh, Ref<Material> material);

	void pushToDescriptorSet(size_t index);
	VkDescriptorSet getDescriptorSet(size_t index);
};

}
