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

private:
	std::vector<Ref<Object>> objects;
	Ref<Camera> camera;
	Ref<Object> root;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Scene);

	// TODO: multiple cameras, camera filtering for different objects (manage render passes? or let the render graph do that)
	// TODO: draw list which the render server queries
	Ref<Camera> getCamera();
	template <class T>
	inline Ref<T> insertObject(Ref<T> obj);
	void removeObject(Ref<Object> obj);
	template <class T>
	inline Ref<T> findObject(std::string name);
	std::vector<Ref<Object>> getAllObjects();

	Scene();
	inline ~Scene() { };
};

template<class T>
inline Ref<T> Scene::insertObject(Ref<T> obj)
{
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
	ref->setParent(root);
	return obj;
}

template<class T>
inline Ref<T> Scene::findObject(std::string name)
{
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
