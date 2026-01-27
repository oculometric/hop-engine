#pragma once

#include <vector>

#include "common.h"
#include "vulkan_typedefs.h"
#include "transform.h"
#include "pbr.h"
#include "draw_command.h"
#include "math_helpers.h"

namespace HopEngine
{

class Object : public Destructible
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
	~Object() override;
	
	virtual std::vector<DrawCommand> getDrawCommands() const;
	Ref<Object> getParent();
	virtual BoundingBox getLocalBounds() const;
	template <class T> void setParent(Ref<T> new_parent);
	template <class T> void setParent(WeakRef<T> new_parent);
	virtual void pushToDescriptorSet(size_t index);

	virtual void drawImGuiDebug();
	
private:
	void _setParent(const Ref<Object>& new_parent);
};

class Camera : public Object
{
public:
	float fov = 90.0f;
	float near_clip = 0.01f;
	float far_clip = 100.0f;
	glm::vec3 clear_colour = { 0.004f, 0.509f, 0.506f };

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Camera);
	Camera();

	VkDescriptorSet getDescriptorSet(size_t index) const;
	SceneUniforms getSceneUniforms(glm::ivec2 viewport_size, float time, const std::vector<LightParams>& lights, glm::vec4 ambient);
	glm::mat4 getWorldToScreenMatrix();
	void pushToDescriptorSet(size_t index) override;
	void pushToCameraDescriptorSet(size_t index, glm::ivec2 viewport_size, float time, const std::vector<LightParams>& lights, glm::vec4 ambient);
	
	void drawImGuiDebug() override;
};

class StaticMesh : public Object
{
public:
	Ref<Mesh> mesh;
	Ref<Material> material;
	uint32_t camera_mask = 0x000000FF;

public:
	DELETE_CONSTRUCTORS(StaticMesh);
	StaticMesh(const Ref<Mesh>& _mesh, const Ref<Material>& _material);

	std::vector<DrawCommand> getDrawCommands() const override;
	BoundingBox getLocalBounds() const override;
	void pushToDescriptorSet(size_t index) override;
	
	void drawImGuiDebug() override;
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
	glm::vec3 colour = { 1, 0, 0 };
	float strength = 1.0f;
	float spot_angle = 0.0f;

public:
	DELETE_CONSTRUCTORS(Light);
	Light(LightType _type);

	LightParams getParamsStructure();
	
	void drawImGuiDebug() override;
};

template<class T>
void Object::setParent(Ref<T> new_parent)
{
	static_assert(std::is_convertible_v<T*, Object*>, "parent must be a HopEngine::Object subclass");
	auto cast = new_parent ? new_parent.template cast<Object>() : Ref<Object>();
	_setParent(cast);
}

template<class T>
void Object::setParent(WeakRef<T> new_parent)
{
	static_assert(std::is_convertible_v<T*, Object*>, "parent must be a HopEngine::Object subclass");
	Ref<T> strong_ref = new_parent;
	setParent(strong_ref);
}

}
