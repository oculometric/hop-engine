#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/vec3.hpp>

#include "common.h"
#include "transform.h"

namespace HopEngine
{

class Object
{
public:
	Transform transform;
	Ref<Mesh> mesh;
	Ref<Material> material;
	Ref<Object> parent;

private:
	Ref<UniformBlock> uniforms;

public:
	DELETE_CONSTRUCTORS(Object);

	Object(Ref<Mesh> mesh, Ref<Material> material);

	void pushToDescriptorSet(size_t index);
	VkDescriptorSet getDescriptorSet(size_t index);

	~Object();
};

// TODO: make camera an object so it can be parented...
class Camera
{
public:
	Transform transform;
	float fov = 90.0f;
	float near_clip = 0.01f;
	float far_clip = 100.0f;

private:
	Ref<UniformBlock> uniforms;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Camera);

	Camera();

	void pushToDescriptorSet(size_t index, glm::ivec2 viewport_size, float time);
	VkDescriptorSet getDescriptorSet(size_t index);

	~Camera();
};

}
