#pragma once

#include <set>
#include <vector>

#include "common.h"
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
	WeakRef<Object> self;
	
private:
	WeakRef<Scene> scene;
	WeakRef<Object> parent;
	std::vector<Ref<Object>> children;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Object);
	static Ref<Object> create();
	~Object() override;
	
	virtual std::vector<DrawCommand> getDrawCommands();
	virtual BoundingBox getLocalBounds() const;
	
	WeakRef<Scene> getScene();
	void setScene(WeakRef<Scene> new_scene);
	void removeFromScene();
	WeakRef<Object> getParent();
	void removeFromParent();
	size_t getChildCount() const;
	Ref<Object> getChild(size_t index);
	void addChild(Ref<Object> object);
	template<class T> void addChild(Ref<T> obj);
	
	virtual void drawImGuiDebug();
	
protected:
	Object();

	void updateObjectUniforms();
};

template <class T>
void Object::addChild(Ref<T> obj)
{
	static_assert(std::is_convertible_v<T*, Object*>, "object must be a HopEngine::Object subclass");
	return addChild(obj.template cast<Object>());
}

class Camera : public Object
{
public:
	float fov = 90.0f;
	float near_clip = 0.01f;
	float far_clip = 100.0f;
	glm::vec3 clear_colour = { 0.004f, 0.509f, 0.506f };

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Camera);
	static Ref<Camera> create();

	void bind(const Ref<DrawCommandBuffer>& command_buffer, glm::ivec2 viewport_size, const std::vector<LightParams>& lights, glm::vec4 ambient);
	SceneUniforms getSceneUniforms(glm::ivec2 viewport_size, const std::vector<LightParams>& lights, glm::vec4 ambient);
	glm::mat4 getWorldToScreenMatrix();
	
	void drawImGuiDebug() override;
	
protected:
	Camera();
};

class StaticMesh : public Object
{
public:
	Ref<Mesh> mesh;
	Ref<Material> material;
	uint32_t camera_mask = 0x000000FF;

public:
	DELETE_CONSTRUCTORS(StaticMesh);
	static Ref<StaticMesh> create(const Ref<Mesh>& _mesh, const Ref<Material>& _material);

	std::vector<DrawCommand> getDrawCommands() override;
	BoundingBox getLocalBounds() const override;
	
	void drawImGuiDebug() override;
	
protected:
	StaticMesh(const Ref<Mesh>& _mesh, const Ref<Material>& _material);
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
	static Ref<Light> create(LightType _type);

	LightParams getParamsStructure();
	
	void drawImGuiDebug() override;
	
protected:
	Light(LightType _type);
};

}
