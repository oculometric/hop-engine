#include "scene.h"

#include <imgui.h>

#include "texture.h"
#include "render_graph.h"
#include "math_helpers.h"

using namespace HopEngine;
using namespace std;

Scene::Scene()
{
	render_graph = new RenderGraph(RenderGraphBuilder().addCamera(0));
	root = new Object();
	root->name = "scene root";
	backup_camera = new Camera();
	backup_camera->setParent(root);
	cameras[0] = new Camera();
	insertObject(cameras[0]);

	DBG_INFO("created new scene");
}

Scene::~Scene()
{
	DBG_INFO("destroying scene " + PTR(this));
}

vector<Ref<Object>> Scene::getAllObjects() const
{ return objects; }

Ref<Camera> Scene::getCamera(const size_t slot) const
{
	const auto it = cameras.find(slot);
	if (it != cameras.end())
		return it->second;
	return backup_camera;
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

vector<DrawCommand> Scene::getDrawCommands() const
{
	vector<DrawCommand> commands;
	for (const Ref<Object>& object : objects)
	{
		auto obj_commands = object->getDrawCommands();
		commands.insert(commands.begin(), obj_commands.begin(), obj_commands.end());
	}
	return commands;
}

WeakRef<Object> Scene::raycast(const glm::vec3 position, const glm::vec3 direction)
{
	float min_dist = INFINITY;
	WeakRef<Object> closest_obj;
	for (auto& object : objects)
	{
		const float result = intersect(position, direction, object->getLocalBounds(), object->transform);
		if (result < 0.01f)
			continue;
		if (result < min_dist)
		{
			min_dist = result;
			closest_obj = object;
		}
	}
	return closest_obj;
}

void Scene::removeObject(Ref<Object> obj)
{
	auto it = objects.begin();
	for (it = objects.begin(); it != objects.end(); ++it)
	{
		if (it->get() == obj.get())
		{
			objects.erase(it);
			break;
		}
	}
	DBG_ERROR("attempt to remove object '" + obj->name + "' (" + PTR(obj.get()) + ") from scene " + PTR(this) + " but it is not present in the tree!");
}

void Scene::setCameraSlot(const Ref<Camera>& camera, const size_t slot)
{ cameras[slot] = camera; }
