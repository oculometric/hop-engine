#include "scene.h"

#include <imgui/imgui.h>

#include "texture.h"
#include "render_graph.h"
#include "math_helpers.h"

using namespace HopEngine;
using namespace std;

Ref<Scene> Scene::create(const std::string& name)
{
	Ref scn = new Scene(name);
	scn->self = scn;
	scn->insertObject(scn->root);
	scn->insertObject(scn->backup_camera.cast<Object>());
	return scn;
}

Scene::~Scene()
{
	DBG_INFO("destroying scene " + PTR(this));
}

vector<WeakRef<Object>> Scene::getAllObjects() const
{
	vector<WeakRef<Object>> objs;
	objs.reserve(objects.size());
	for (auto& obj : objects)
		objs.emplace_back(obj);
	return objs;
}

Ref<Object> Scene::findObject(const std::string& name) const
{
	for (auto& test_obj : objects)
	{
		if (test_obj->name == name)
			return test_obj;
	}
	return nullptr;
}

Ref<Object> Scene::insertObject(Ref<Object> obj)
{
	// if object in set, return
	auto it = objects.begin();
	while (it != objects.end() && it->get() != obj.get())
		++it;
	if (it != objects.end())
		return obj;
	// otherwise add object to set
	objects.emplace_back(obj);
	if (dynamic_cast<Light*>(obj.get()) != nullptr)
	{
		auto ref2 = obj.template cast<Light>();
		lights.emplace_back(ref2);
	}
	// and call object set scene
	obj->setScene(self);
	// if object parent is null, set parent to root
	if (!obj->getParent())
		root->addChild(obj);
	return obj;
}

void Scene::removeObject(Ref<Object> obj)
{
	// if object not in set, return
	auto it = objects.begin();
	while (it != objects.end() && it->get() != obj.get())
		++it;
	if (it == objects.end())
		return;
	// otherwise remove object from set
	objects.erase(it);
	if (dynamic_cast<Light*>(obj.get()) != nullptr)
	{
		auto ref2 = obj.template cast<Light>();
		auto it2 = lights.begin();
		while (it2 != lights.end() && it2->get() != ref2.get())
			++it2;
		lights.erase(it2);
	}
	// and call object remove from scene
	obj->removeFromScene();
}

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

WeakRef<Object> Scene::raycast(const glm::vec3 position, const glm::vec3 direction) const
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

void Scene::setCameraSlot(const Ref<Camera>& camera, const size_t slot)
{ cameras[slot] = camera; }

void Scene::updateUniforms(uint32_t image_index, float time_since_start, glm::u32vec2 viewport_size, FrameStats& stats)
{
	last_viewport_size = viewport_size;
	if (!render_graph)
		return;
	
	stats.lights = getLightParams().size();
	const glm::u32vec2 graph_extent = render_graph->getExpectedExtent();
	if (graph_extent.x != viewport_size.x || graph_extent.y != viewport_size.y)
		render_graph->resizeBuffers(viewport_size.x, viewport_size.y);
	render_graph->updateUniforms(image_index, time_since_start, WeakRef<Scene>(this));

	for (auto& object : objects)
		object->pushToDescriptorSet(image_index);
}

Scene::Scene(const string& name)
{
	origin = name;
	render_graph = new RenderGraph(RenderGraphBuilder().addCamera(0));
	root = Object::create();
	root->name = "scene root";
	backup_camera = Camera::create();

	DBG_INFO("created new scene " + getOrigin());
}
