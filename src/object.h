#pragma once

#include <vulkan/vulkan.h>
#include <glm/vec3.hpp>

#include "common.h"
#include "transform.h"

namespace HopEngine
{

class Material;
class Mesh;
class UniformBlock;

class Object
{
public:
	Transform transform;
	Ref<Mesh> mesh = nullptr;
	Ref<Material> material = nullptr;
	Ref<Object> parent = nullptr;

private:
	Ref<UniformBlock> uniforms = nullptr;

public:
	DELETE_CONSTRUCTORS(Object);

	Object(Ref<Mesh> mesh, Ref<Material> material);

	void pushToDescriptorSet(size_t index);
	VkDescriptorSet getDescriptorSet(size_t index);
};

}
