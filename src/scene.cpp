#include "scene.h"

using namespace HopEngine;
using namespace std;

Ref<Camera> Scene::getCamera()
{
	return camera;
}

void Scene::removeObject(Ref<Object> obj)
{
	auto it = objects.begin();
	for (it = objects.begin(); it != objects.end(); ++it)
	{
		if ((*it).get() == obj.get())
		{
			objects.erase(it);
			break;
		}
	}
	DBG_WARNING("attempt to remove object " + PTR(obj.get()) + " from scene " + PTR(this) + " but it is not present in the tree!");
}

vector<Ref<Object>> Scene::getAllObjects()
{
	return objects;
}

Scene::Scene()
{
	camera = new Camera();
}
