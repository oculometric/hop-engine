#include "scene.h"

#include <imgui.h>

#include "texture.h"
#include "render_graph.h"
#include "uniform_block.h"

using namespace HopEngine;
using namespace std;

Ref<Camera> Scene::getCamera(size_t slot) const
{
	auto it = cameras.find(slot);
	if (it != cameras.end())
		return it->second;
	else
		return backup_camera;
}

void Scene::setCameraSlot(Ref<Camera> camera, size_t slot)
{
	cameras[slot] = camera;
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

Ref<RenderGraph> Scene::getRenderGraph() const
{
	return render_graph;
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

Scene::Scene()
{
	render_graph = new RenderGraph(RenderGraphBuilder().addCamera(0));
	root = new Object();
	root->name = "scene root";
	backup_camera = new Camera();
	backup_camera->setParent(root);
	cameras[0] = new Camera();
	cameras[0]->setParent(root);

	DBG_INFO("created new scene");
}

Scene::~Scene()
{
	DBG_INFO("destroying scene " + PTR(this));
}
