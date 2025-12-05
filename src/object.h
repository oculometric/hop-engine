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

private:
	Ref<Mesh> mesh;
	Ref<Material> material;
	Ref<UniformBlock> uniforms;

public:
	DELETE_CONSTRUCTORS(Object);

	Object(Ref<Mesh> mesh, Ref<Material> material);

	Ref<Mesh> getMesh();
	Ref<Material> getMaterial();
	void pushToDescriptorSet(size_t index);
	VkDescriptorSet getDescriptorSet(size_t index);
};

}
