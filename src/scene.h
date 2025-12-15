#pragma once

#include <vector>
#include <set>

#include "common.h"
#include "object.h"

namespace HopEngine
{

class Scene
{
public:
	glm::vec3 background_colour = { 0.004f, 0.509f, 0.506f };
	glm::vec3 ambient_colour = { 0.01f, 0.01f, 0.01f };

private:
	std::vector<Ref<Object>> objects;
	Ref<Camera> camera;
	Ref<Object> root;
	std::vector<Ref<Light>> lights;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Scene);

	// TODO: multiple cameras, camera filtering for different objects (manage render passes? or let the render graph do that)
	// TODO: draw list which the render server queries
	Ref<Camera> getCamera() const;
	template <class T>
	inline Ref<T> insertObject(Ref<T> obj);
	void removeObject(Ref<Object> obj);
	template <class T>
	inline Ref<T> findObject(std::string name) const;
	std::vector<Ref<Object>> getAllObjects() const;
	std::vector<LightParams> getLightParams() const;

	Scene();
	inline ~Scene() { };
};

template<class T>
inline Ref<T> Scene::insertObject(Ref<T> obj)
{
	static_assert(std::is_convertible<T*, Object*>::value, "object must be a HopEngine::Object subclass");
	if (obj.get() == root.get())
	{
		DBG_ERROR("attempt to insert object " + PTR(obj.get()) + " into scene " + PTR(this) + " but it is already present in the tree!");
		return nullptr;
	}

	for (auto& test_obj : objects)
	{
		if (test_obj.get() == obj.get())
		{
			DBG_ERROR("attempt to insert object " + PTR(obj.get()) + " into scene " + PTR(this) + " but it is already present in the tree!");
			return obj;
		}
	}

	auto ref = obj.template cast<Object>();
	objects.push_back(ref);
	if (!ref->getParent()) ref->setParent(root);
	if (std::is_convertible<T*, Light*>::value)
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
	for (auto& test_obj : objects)
	{
		if (test_obj->name == name)
		{
			return test_obj.cast<T>();
		}
	}
	return nullptr;
}

}
