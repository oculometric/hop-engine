#include "scene.h"

#include "texture.h"

using namespace HopEngine;
using namespace std;

Ref<Camera> Scene::getCamera() const
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
	DBG_ERROR("attempt to remove object " + PTR(obj.get()) + " from scene " + PTR(this) + " but it is not present in the tree!");
}

vector<Ref<Object>> Scene::getAllObjects() const
{
	return objects;
}

vector<LightParams> Scene::getLightParams() const
{
	vector<LightParams> lights_params(8);
	
	size_t index = 0;
	for (const auto& light : lights)
	{
		lights_params[index] = light->getParamsStructure();
		++index;
		if (index >= 8)
			break;
	}

	return lights_params;
}

Scene::Scene()
{
	camera = new Camera();
	root = new Object();
}
