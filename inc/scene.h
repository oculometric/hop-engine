#pragma once

#include <vector>
#include <map>

#include "common.h"
#include "object.h"
#include "draw_command.h"

namespace HopEngine
{

class Scene : public Destructible
{
public:
	glm::vec3 ambient_colour = { 0.01f, 0.01f, 0.01f };
	Ref<Texture> skybox;
	Ref<RenderGraph> render_graph;

private:
	std::vector<Ref<Object>> objects;
	std::map<size_t, Ref<Camera>> cameras;
	Ref<Camera> backup_camera;
	Ref<Object> root;
	std::vector<Ref<Light>> lights;
	std::string origin;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Scene);

	Ref<Camera> getCamera(size_t slot) const;
	void setCameraSlot(Ref<Camera> camera, size_t slot);
	template <class T>
	inline Ref<T> insertObject(Ref<T> obj);
	void removeObject(Ref<Object> obj);
	template <class T>
	inline Ref<T> findObject(std::string name) const;
	std::vector<Ref<Object>> getAllObjects() const;
	std::vector<LightParams> getLightParams() const;
	Ref<RenderGraph> getRenderGraph() const;
	std::vector<DrawCommand> getDrawCommands() const;
	WeakRef<Object> raycast(glm::vec3 origin, glm::vec3 direction);
	inline std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }
	
	void drawImGuiDebug();
	
	static Ref<Scene> deserialise(std::string name);

	Scene();
	~Scene() override;
};

template<class T>
inline Ref<T> Scene::insertObject(Ref<T> obj)
{
	static_assert(std::is_convertible<T*, Object*>::value, "object must be a HopEngine::Object subclass");
	if (obj.get() == root.get())
	{
		DBG_ERROR("attempt to insert object '" + obj->name + "' (" + PTR(obj.get()) + ") into scene " + PTR(this) + " but it is already present in the tree!");
		return nullptr;
	}

	for (auto& test_obj : objects)
	{
		if (test_obj.get() == obj.get())
		{
			DBG_ERROR("attempt to insert object '" + obj->name + "' (" + PTR(obj.get()) + ") into scene " + PTR(this) + " but it is already present in the tree!");
			return obj;
		}
	}

	auto ref = obj.template cast<Object>();
	objects.push_back(ref);
	if (!ref->getParent()) ref->setParent(root);
	if (dynamic_cast<Light*>(obj.get()) != nullptr)
	{
		auto ref2 = obj.template cast<Light>();
		lights.push_back(ref2);
	}
	return obj;
}

template<class T>
inline Ref<T> Scene::findObject(std::string name) const
{
	static_assert(std::is_convertible<T*, Object*>::value, "expected type must be a HopEngine::Object subclass");
	for (auto test_obj : objects)
	{
		if (test_obj->name == name)
		{
			return test_obj.cast<T>();
		}
	}
	return nullptr;
}

}
