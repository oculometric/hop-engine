#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>

#include "common.h"
#include "transform.h"
#include "pbr.h"

namespace HopEngine
{

// TODO: per-object camera bitmask

class Object
{
public:
	std::string name;
	Transform transform;
	glm::vec3 clear_colour = { 0.004f, 0.509f, 0.506f };

protected:
	Ref<UniformBlock> uniforms;
	
private:
	Ref<Object> parent;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Object);

	Object();

	template <class T>
	void setParent(Ref<T> new_parent);
	template <class T>
	void setParent(WeakRef<T> new_parent);
	Ref<Object> getParent();

	virtual void pushToDescriptorSet(size_t index);
	virtual std::vector<DrawCommand> getDrawCommands() const;

	virtual ~Object();

private:
	void _setParent(Ref<Object> new_parent);
};

class Camera : public Object
{
public:
	float fov = 90.0f;
	float near_clip = 0.01f;
	float far_clip = 100.0f;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Camera);

	Camera();

	void pushToCameraDescriptorSet(size_t index, glm::ivec2 viewport_size, float time, std::vector<LightParams> lights, glm::vec4 ambient);
	glm::mat4 getWorldToScreenMatrix();
	VkDescriptorSet getDescriptorSet(size_t index) const;
};

class StaticMesh : public Object
{
public:
	Ref<Mesh> mesh;
	Ref<Material> material;

public:
	DELETE_CONSTRUCTORS(StaticMesh);

	StaticMesh(Ref<Mesh> mesh, Ref<Material> material);

	void pushToDescriptorSet(size_t index) override;
	std::vector<DrawCommand> getDrawCommands() const override;
};

class Light : public Object
{
public:
	enum LightType
	{
		POINT,
		SPOT,
		DIRECTIONAL
	};

public:
	LightType type;
	glm::vec4 colour = { 1, 0, 0, 0 };
	float spot_angle = 0.0f;

public:
	DELETE_CONSTRUCTORS(Light);

	Light(LightType type);

	LightParams getParamsStructure() const;
};

template<class T>
inline void Object::setParent(Ref<T> new_parent)
{
	static_assert(std::is_convertible<T*, Object*>::value, "parent must be a HopEngine::Object subclass");
	auto cast = new_parent.template cast<Object>();
	_setParent(cast);
}

template<class T>
inline void Object::setParent(WeakRef<T> new_parent)
{
	static_assert(std::is_convertible<T*, Object*>::value, "parent must be a HopEngine::Object subclass");
	Ref<T> strong_ref = new_parent;
	setParent(strong_ref);
}

}
