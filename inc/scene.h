#pragma once

#include <vector>
#include <map>

#include "common.h"
#include "draw_command.h"
#include "math_helpers.h"

namespace HopEngine
{

struct FrameStats;

class Object;

class Component : public Destructible
{
	friend class Object;
private:
	WeakRef<Object> owner;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Component);
	~Component() override { }

	// TODO: components can be enabled/disabled
	
	WeakRef<Object> getOwner() const { return owner; }
	template<class T> WeakRef<T> getComponent();
	WeakRef<Scene> getScene() const;
	Transform& getTransform() const;

	virtual void awake() { }
	virtual void update(float delta_time) { }

	virtual std::vector<DrawCommand> getDrawCommands() { return { }; }
	virtual BoundingBox getLocalBounds() const { return BoundingBox{ }; }

	virtual void drawImGuiDebug();
private:
	Component() = default;
};

class Object final : public Destructible
{
	friend class Scene;
public:
	std::string name = "object";
	Transform transform;

private:
	WeakRef<Object> self;
	WeakRef<Scene> scene;
	WeakRef<Object> parent;
	std::vector<Ref<Object>> children;
	std::vector<Ref<Component>> components;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Object);
	static Ref<Object> create();
	~Object() override { }
	
	WeakRef<Object> getSelf() const { return self; }
	WeakRef<Scene> getScene() const { return scene; }
	WeakRef<Object> getParent() const { return parent; }
	size_t getChildCount() const { return children.size(); }
	WeakRef<Object> getChild(size_t index) const { return children[index]; }
	void removeFromParent();
	void addChild(Ref<Object> obj);
	
	template<class T> WeakRef<T> addComponent()
	{
		static_assert(std::is_convertible_v<T*, Component*>, "component must be a HopEngine::Component subclass");
		Ref<T> comp = new T();
		components.push_back(comp);
		comp->awake();
		return comp.weak();
	}
	template<class T> WeakRef<T> getComponent()
	{
		static_assert(std::is_convertible_v<T*, Component*>, "component must be a HopEngine::Component subclass");
		for (const auto& comp : components)
		{
			if (dynamic_cast<T>(comp.get()))
				return comp;
		}
		return WeakRef<T>();
	}
	template<class T> bool removeComponent()
	{
		static_assert(std::is_convertible_v<T*, Component*>, "component must be a HopEngine::Component subclass");
		for (auto it = components.begin(); it != components.end(); ++it)
		{
			if (dynamic_cast<T>((*it).get()))
			{
				components.erase(it);
				return true;
			}
		}
		return false;
	}

	void update(float delta_time);
	std::vector<DrawCommand> getDrawCommands();
	BoundingBox getLocalBounds() const;
	
	void drawImGuiDebug();
	
private:
	Object() = default;
};

template<class T> WeakRef<T> Component::getComponent()
{
	return owner->getComponent<T>();
}

class Scene final : public Destructible
{
public:
	glm::vec3 ambient_colour = { 0.01f, 0.01f, 0.01f };
	Ref<RenderGraph> render_graph;

private:
	std::string origin;
	WeakRef<Scene> self;
	Ref<Object> root;
	std::vector<Ref<Object>> objects;
	glm::u32vec2 last_viewport_size;
	Ref<Material> skybox_material; // TODO
	WeakRef<Texture> skybox;

public:
	DELETE_CONSTRUCTORS(Scene);
	static Ref<Scene> create(const std::string& name = "scene");
	~Scene() override;
	
	std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }

	std::vector<WeakRef<Object>> getAllObjects() const;
	WeakRef<Object> findObject(const std::string& name) const;
	WeakRef<Object> insertObject(Ref<Object> obj);
	WeakRef<Object> insertObject(const std::string& name);
	void removeObject(WeakRef<Object> obj);
	
	glm::u32vec2 getViewportSize() const { return last_viewport_size; }
	WeakRef<Object> raycast(glm::vec3 position, glm::vec3 direction) const;

	void setSkybox(WeakRef<Texture> texture); // TODO

	void update(float delta_time);
	void draw(Ref<DrawCommandBuffer> command_buffer, glm::u32vec2 viewport_size); // TODO: gather cameras (set their uniforms), gather lights, gather draw calls, resize and draw render graph
	void bindOutputMaterial(Ref<DrawCommandBuffer> command_buffer);

	static Ref<Scene> deserialise(const std::string& name); // TODO:
	
	void drawImGuiDebug();
	
private:
	Scene(const std::string& name);
};

}
