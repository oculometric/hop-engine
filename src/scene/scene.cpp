#include "scene.h"

#include <imgui/imgui.h>

#include "texture.h"
#include "render_graph.h"
#include "math_helpers.h"
#include "command_buffer.h"
#include "basic_components.h"
#include "render_server.h"

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

void Object::addChild(WeakRef<Object> obj)
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
			children.emplace_back(obj.strong());
			return;
		}
	}
	DBG_WARNING("object " + name + " claims to be a child of object " + parent->name + ", but could not be found in the parents child list.");
}

void Object::update(float delta_time)
{
	for (const auto& comp : components)
    {
        if (!comp->getEnabled())
            continue;
		comp->update(delta_time);
    }
	for (const auto& child : children)
		child->update(delta_time);
}

vector<DrawCommand> Object::getDrawCommands()
{
	vector<DrawCommand> commands;
	for (const auto& comp : components)
	{
        if (!comp->getEnabled())
            continue;
		auto sub_commands = comp->getDrawCommands();
		commands.insert(commands.begin(), sub_commands.begin(), sub_commands.end());
	}
	return commands;
}

BoundingBox Object::getLocalBounds() const
{
	return BoundingBox{ { 0, 0, 0 }, { 0.1f, 0.1f, 0.1f } };
}

Object::Object()
{
	transform.owner = this;
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
	DBG_INFO("destroying scene " + getOrigin());
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

WeakRef<Object> Scene::insertObject(WeakRef<Object> obj)
{
	if (obj->scene)
	{
		if (obj->scene == self)
		{
			if (!obj->parent)
			{
				obj->parent = root;
				root->children.emplace_back(obj.strong());
				return obj;
			}
			return obj;
		}
		obj->scene->removeObject(obj);
	}
	objects.emplace_back(obj.strong());
	obj->scene = self;
	if (!obj->parent)
	{
		obj->parent = root;
		root->children.emplace_back(obj.strong());
	}
	for (const auto& child : obj->children)
		insertObject(child);
	return obj;
}

WeakRef<Object> Scene::addObject(const string& name)
{
	Ref obj = Object::create();
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

WeakRef<Object> Scene::raycast(const glm::vec3 position, const glm::vec3 direction) const
{
	float min_dist = INFINITY;
	WeakRef<Object> closest_obj;
	for (auto& object : objects)
	{
		const float result = intersect(position, direction, object->getLocalBounds(), object->transform.getMatrix());
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

void Scene::setSkybox(WeakRef<Texture> texture)
{
	if (texture == skybox)
		return;
	skybox = texture;
	skybox_material->setTexture("tex", texture.strong());
}

void Scene::update(float delta_time)
{
	root->update(delta_time);
}

void Scene::draw(Ref<DrawCommandBuffer> command_buffer, glm::u32vec2 viewport_size)
{
	last_viewport_size = viewport_size;
	if (!render_graph)
		return;

	// resize render graph
	render_graph->resizeBuffers(viewport_size);

	// collect all lights from the scene
	vector<LightParams> lights(8);
	
	size_t index = 0;
	for (const auto& light : objects)
	{
		auto light_comp = light->getComponent<LightComponent>();
		if (!light_comp)
			continue;
		lights[index] = light_comp->getParamsStructure();
		++index;
		if (index >= 8)
			break;
	}

	// check the size of each camera slot
	map<size_t, pair<WeakRef<UniformBlock>, glm::vec4>> cameras;
	auto camera_sizes = render_graph->getCameraSlots();
	for (const auto& object : objects)
	{
		auto camera_comp = object->getComponent<CameraComponent>();
		if (!camera_comp)
			continue;
		// find the first camera for each slot, and update its uniforms to be correct (and store)
		cameras[camera_comp->camera_slot] = { camera_comp->getUniforms(camera_sizes[camera_comp->camera_slot], lights, glm::vec4(ambient_colour, 0)), glm::vec4(camera_comp->clear_colour, 1) };
	}

	// collect all draw calls from the scene (plus the skybox!)
	vector<DrawCommand> draw_commands;
    if (skybox)
	    draw_commands.push_back(DrawCommand(skybox_material, RenderServer::getSkyboxCube(), skybox_uniforms).priority(1000));
	for (const auto& object : objects)
	{
		auto temp = object->getDrawCommands();
		draw_commands.insert(draw_commands.end(), temp.begin(), temp.end());
	}

	// call render graph draw with the cameras and draw calls
	render_graph->draw(command_buffer, draw_commands, cameras);
}

void Scene::bindOutputMaterial(Ref<DrawCommandBuffer> command_buffer)
{
	if (render_graph)
		render_graph->bindOutputMaterial(command_buffer);
}

Scene::Scene(const string& name)
{
	origin = name;
	render_graph = new RenderGraph(RenderGraph::Builder().addCamera(0));
	root = Object::create();
    skybox_material = new Material(new Shader("res://engine/shaders/skybox.glsl"), Pipeline::Builder().cullMode(Pipeline::CULL_NONE).depthWrite(false).depthTest(false));
	skybox_uniforms = RenderServer::createObjectUniforms();
	root->name = "scene root";

	DBG_INFO("created new scene " + getOrigin());
}

bool DrawCommand::operator()(const DrawCommand& a, const DrawCommand& b) const
{
    return DrawCommand::compare(a, b);
}

bool DrawCommand::compare(const DrawCommand& a, const DrawCommand& b)
{
    if (a.draw_priority <= b.draw_priority)
        return false;
    if (a.material->getShader() > b.material->getShader())
        return false;
    if (a.material > b.material)
        return false;
    if (a.uniforms > b.uniforms)
        return false;
    if (a.mesh > b.mesh)
        return false;
    return true;
}
