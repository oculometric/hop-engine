#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/vec3.hpp>
#include <vector>

#include "common.h"
#include "transform.h"

namespace HopEngine
{

class Object
{
public:
	std::string name;
	Transform transform;

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

	void pushToDescriptorSet(size_t index, glm::ivec2 viewport_size, float time);
	VkDescriptorSet getDescriptorSet(size_t index);
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

template<class T>
inline void Object::setParent(Ref<T> new_parent)
{
	static_assert(std::is_convertible<T*, Object*>::value, "parent must be a HopEngine::Object subclass");
	auto cast = new_parent.cast<Object>();
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
