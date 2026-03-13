#include "scene.h"

#include <imgui/imgui.h>

#include "texture.h"
#include "render_graph.h"
#include "math_helpers.h"

using namespace HopEngine;
using namespace std;

WeakRef<Scene> Component::getScene() const
{ return owner->getScene(); }

Transform& Component::getTransform() const
{ return owner->transform; }

Ref<Object> Object::create()
{
	Ref obj = new Object();
	obj->self = obj;
	return obj;
}

void Object::removeFromParent()
{
	if (!parent)
		return;
	for (auto it = parent->children.begin(); it != parent->children.end(); ++it)
	{
		if ((*it).get() == this)
		{
			parent->children.erase(it);
			parent = nullptr;
			if (scene)
				scene->insertObject(self.strong());
			return;
		}
	}
	DBG_WARNING("object " + name + " claims to be a child of object " + parent->name + ", but could not be found in the parents child list.");
}

void Object::addChild(Ref<Object> obj)
{
	if (obj->scene && obj->scene != scene)
	{
		DBG_WARNING("object " + obj->name + " is already present in another scene hierarchy.");
		return;
	}
	if (scene)
		scene->insertObject(obj);
	for (auto it = obj->parent->children.begin(); it != obj->parent->children.end(); ++it)
	{
		if ((*it).get() == obj.get())
		{
			obj->parent->children.erase(it);
			obj->parent = self;
			children.emplace_back(obj);
			return;
		}
	}
	DBG_WARNING("object " + name + " claims to be a child of object " + parent->name + ", but could not be found in the parents child list.");
}

void Object::update(float delta_time)
{
	for (const auto& comp : components)
		comp->update(delta_time);
	for (const auto& child : children)
		child->update(delta_time);
}

vector<DrawCommand> Object::getDrawCommands()
{
	vector<DrawCommand> commands;
	for (const auto& comp : components)
	{
		auto sub_commands = comp->getDrawCommands();
		commands.insert(commands.begin(), sub_commands.begin(), sub_commands.end());
	}
	return commands;
}

BoundingBox Object::getLocalBounds() const
{
	return BoundingBox{ { 0, 0, 0 }, { 0.1f, 0.1f, 0.1f } };
}

Ref<Scene> Scene::create(const std::string& name)
{
	Ref scn = new Scene(name);
	scn->self = scn;
	scn->root->scene = scn;
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

WeakRef<Object> Scene::findObject(const std::string& name) const
{
	for (auto& test_obj : objects)
	{
		if (test_obj->name == name)
			return test_obj;
	}
	return nullptr;
}

WeakRef<Object> Scene::insertObject(Ref<Object> obj)
{
	if (obj->scene)
	{
		if (obj->scene == self)
		{
			if (!obj->parent)
			{
				obj->parent = root;
				root->children.emplace_back(obj);
				return obj;
			}
			DBG_WARNING("object " + obj->name + " is already a member of scene " + getOrigin());
			return obj;
		}
		obj->scene->removeObject(obj);
	}
	objects.emplace_back(obj);
	obj->scene = self;
	if (!obj->parent)
	{
		obj->parent = root;
		root->children.emplace_back(obj);
	}
	for (const auto& child : obj->children)
		insertObject(child);
	return obj;
}

WeakRef<Object> Scene::insertObject(const string& name)
{
	Ref obj = new Object();
	obj->name = name;
	obj->scene = self;
	objects.emplace_back(obj);
	obj->parent = root;
	root->children.emplace_back(obj);
	return obj;
}

void Scene::removeObject(WeakRef<Object> obj)
{
	if (obj->scene != self)
	{
		DBG_WARNING("object " + obj->name + " is not a member of scene " + getOrigin());
		return;
	}
	for (auto it = objects.begin(); it != objects.end(); ++it)
	{
		if ((*it).get() == obj.get())
		{
			objects.erase(it);
			obj->scene = nullptr;
			if (obj->parent)
			{
				if (obj->parent->scene)
				{
					for (auto it2 = obj->parent->children.begin(); it2 != obj->parent->children.end(); ++it2)
					{
						if ((*it2).get() == obj.get())
						{
							obj->parent->children.erase(it2);
							obj->parent = nullptr;
							break;
						}
					}
					if (obj->parent)
						DBG_WARNING("object " + obj->name + " claims to be a child of object " + obj->parent->name + ", but could not be found in the parents child list.");
				}
				for (const auto& child : obj->children)
					removeObject(child);
			}
			else
				DBG_WARNING("object " + obj->name + " has no parent. this is not allowed.");
			break;
		}
	}
	DBG_WARNING("object " + obj->name + " claims to be a member of scene " + getOrigin() + ", but is not known to the scene. scene hierarchy is corrupt.");
}



// Ref<CameraComponent> Scene::getCamera(const size_t slot) const
// {
// 	const auto it = cameras.find(slot);
// 	if (it != cameras.end())
// 		return it->second;
// 	return backup_camera;
// }

// vector<LightParams> Scene::getLightParams() const
// {
// 	vector<LightParams> lights_params(8);
	
// 	size_t index = 0;
// 	for (const auto& light : lights)
// 	{
// 		lights_params[index] = light->getParamsStructure();
// 		++index;
// 		if (index >= 8)
// 			break;
// 	}

// 	return lights_params;
// }

// vector<DrawCommand> Scene::getDrawCommands(glm::u32vec2 viewport_size)
// {
// 	last_viewport_size = viewport_size;
// 	if (!render_graph)
// 		return { };
	
// 	const glm::u32vec2 graph_extent = render_graph->getExpectedExtent();
// 	if (graph_extent.x != viewport_size.x || graph_extent.y != viewport_size.y)
// 		render_graph->resizeBuffers(viewport_size.x, viewport_size.y);

// 	vector<DrawCommand> commands;
// 	for (const Ref<Object>& object : objects)
// 	{
// 		auto obj_commands = object->getDrawCommands();
// 		commands.insert(commands.begin(), obj_commands.begin(), obj_commands.end());
// 	}
// 	return commands;
// }

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

void Scene::update(float delta_time)
{
	root->update(delta_time);
}

void Scene::draw(Ref<DrawCommandBuffer> command_buffer, glm::u32vec2 viewport_size)
{
	if (!render_graph)
		return;
	// resize render graph
	render_graph->resizeBuffers(viewport_size);
	// check the size of each camera slot
	assert(false);
	// TODO HERE
	// find the first camera for each slot, and update its uniforms to be correct (and store)
	// collect all lights from the scene
	// collect all draw calls from the scene (plus the skybox!)
	// call render graph draw with the cameras and draw calls
}

void Scene::bindOutputMaterial(Ref<DrawCommandBuffer> command_buffer)
{
	if (render_graph)
		render_graph->bindOutputMaterial(command_buffer);
}

Scene::Scene(const string& name)
{
	origin = name;
	render_graph = new RenderGraph(RenderGraphBuilder().addCamera(0));
	root = Object::create();
	root->name = "scene root";

	DBG_INFO("created new scene " + getOrigin());
}
