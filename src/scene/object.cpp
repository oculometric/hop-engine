#include "object.h"

#include <glm/gtc/matrix_transform.hpp>

#include "mesh.h"
#include "material.h"
#include "uniform_block.h"
#include "render_server.h"
#include "pbr.h"
#include "command_buffer.h"
#include "scene.h"
#include "engine.h"

using namespace HopEngine;
using namespace std;

Ref<Object> Object::create()
{
	Ref obj = new Object();
	obj->self = obj;
	return obj;
}

Object::~Object()
{
	DBG_VERBOSE("destroying object '" + name + "' (" + PTR(this) + ')');
}

vector<DrawCommand> Object::getDrawCommands()
{
	updateObjectUniforms();
	return { };
}

BoundingBox Object::getLocalBounds() const
{
	return BoundingBox{ { 0, 0, 0 }, { 0.25f, 0.25f, 0.25f } };
}

WeakRef<Scene> Object::getScene()
{
	return scene;
}

void Object::setScene(WeakRef<Scene> new_scene)
{
	if (scene == new_scene)
		return;
	if (scene)
		removeFromScene();
	if (!new_scene)
		return;
	scene = new_scene;
	// add ourselves to the new scene
	new_scene->insertObject(self.strong());
	// set the scene for all our children
	for (auto& child : children)
		child->setScene(new_scene);
}

void Object::removeFromScene()
{
	// if scene is null, return
	if (!scene)
		return;
	// set scene to null
	WeakRef<Scene> _scene = scene;
	scene = nullptr;
	// call scene remove object
	_scene->removeObject(self.strong());
	// if parent scene is not null, call remove from parent
	if (parent && parent->scene)
		removeFromParent();
	// call remove from scene on all children
	for (auto& child : children)
		child->removeFromScene();
}

WeakRef<Object> Object::getParent()
{
	return parent;
}

void Object::removeFromParent()
{
	// if parent is null, return
	if (!parent)
		return;
	// remove child on parent
	auto it = parent->children.begin();
	while (it != parent->children.end() && it->get() != self.get())
		++it;
	if (it == parent->children.end())
	{
		DBG_ERROR("corrupted scene tree!! uh oh");
		return;
	}
	parent->children.erase(it);
	auto mat = transform.getMatrix();
	// set parent to null
	parent = nullptr;
	// if scene is null, return
	// set parent to scene root
	if (scene)
		scene->insertObject(self.strong());
	transform.parent_transform = nullptr;
	transform.setMatrix(mat);
}

size_t Object::getChildCount() const
{
	return children.size();
}

Ref<Object> Object::getChild(size_t index)
{
	if (index >= children.size())
		return nullptr;
	return children[index];
}

void Object::addChild(Ref<Object> object)
{
	if (object.get() == self.get())
	{
		DBG_WARNING("you tried to make an object a child of itself. don't do that!");
		return;
	}
	// if child is in set, return
	auto it = children.begin();
	while (it != children.end() && it->get() != object.get())
		++it;
	if (it != children.end())
		return;
	// add child to set
	children.emplace_back(object);
	// if child scene is not scene, call remove from scene on child, then call insert object on scene + child
	auto mat = object->transform.getMatrix();
	if (object->scene != scene)
	{
		object->removeFromScene();
		if (scene)
			scene->insertObject(object);
	}
	object->transform.parent_transform = &transform;
	object->transform.setMatrix(mat);
	// set parent on child
	object->parent = self;
}

Object::Object()
{
	transform = Transform();
	uniforms = RenderServer::createObjectUniforms();
	name = "object";

	DBG_VERBOSE("created object");
}

void Object::updateObjectUniforms()
{
	ObjectUniforms* object_uniforms = static_cast<ObjectUniforms*>(uniforms->getBuffer());

	object_uniforms->id = static_cast<int>(reinterpret_cast<size_t>(this));
	object_uniforms->model_to_world = transform.getMatrix();
}

Ref<Camera> Camera::create()
{
	Ref obj = new Camera();
	obj->self = obj.cast<Object>();
	return obj;
}

void Camera::bind(const Ref<DrawCommandBuffer>& command_buffer, const glm::ivec2 viewport_size, const vector<LightParams>& lights, const glm::vec4 ambient)
{
	SceneUniforms scene_uniforms = getSceneUniforms(viewport_size, lights, ambient);

	memcpy(uniforms->getBuffer(), &scene_uniforms, sizeof(SceneUniforms));
	uniforms->bind(command_buffer, 0);
}

SceneUniforms Camera::getSceneUniforms(const glm::ivec2 viewport_size, const vector<LightParams>& lights, const glm::vec4 ambient)
{
	SceneUniforms scene_uniforms;
	scene_uniforms.time = Engine::getEngineTime();
	scene_uniforms.eye_position = transform.getPosition();
	scene_uniforms.viewport_size = viewport_size;
	scene_uniforms.world_to_view = glm::inverse(transform.getMatrix());
	scene_uniforms.view_to_clip = glm::perspective(glm::radians(fov), static_cast<float>(viewport_size.x) / static_cast<float>(viewport_size.y), near_clip, far_clip);
	scene_uniforms.view_to_clip[1][1] *= -1;
	scene_uniforms.clip_to_view = glm::inverse(scene_uniforms.view_to_clip);
	scene_uniforms.view_to_world = transform.getMatrix();
	scene_uniforms.near_far = { near_clip, far_clip };
	memcpy(scene_uniforms.lights, lights.data(), lights.size() * sizeof(LightParams));
	scene_uniforms.ambient_light = ambient;

	return scene_uniforms;
}

glm::mat4 Camera::getWorldToScreenMatrix()
{
	const glm::vec2 viewport_size = RenderServer::getFramebufferSize();
	glm::mat4 view_to_clip = glm::perspective(glm::radians(fov), viewport_size.x / static_cast<float>(viewport_size.y), near_clip, far_clip);
	view_to_clip[1][1] *= -1;
	glm::mat4 world_to_view = glm::inverse(transform.getMatrix());
	world_to_view[0] = glm::normalize(world_to_view[0]);
	world_to_view[1] = glm::normalize(world_to_view[1]);
	world_to_view[2] = glm::normalize(world_to_view[2]);
	return view_to_clip * world_to_view;
}

Camera::Camera() : Object()
{
	uniforms = RenderServer::createSceneUniforms();
	name = "camera";
}

Ref<StaticMesh> StaticMesh::create(const Ref<Mesh>& _mesh, const Ref<Material>& _material)
{
	Ref obj = new StaticMesh(_mesh, _material);
	obj->self = obj.cast<Object>();
	return obj;
}

vector<DrawCommand> StaticMesh::getDrawCommands()
{
	updateObjectUniforms();
	vector<DrawCommand> commands;
	if (material && mesh && uniforms)
		commands.push_back(DrawCommand(material, mesh, uniforms).mask(camera_mask));
	return commands;
}

BoundingBox StaticMesh::getLocalBounds() const
{
	return mesh->getBoundingBox();
}

StaticMesh::StaticMesh(const Ref<Mesh>& _mesh, const Ref<Material>& _material) : Object()
{
	mesh = _mesh;
	material = _material;
	name = "static mesh";
}

Ref<Light> Light::create(LightType _type)
{
	Ref obj = new Light(_type);
	obj->self = obj.cast<Object>();
	return obj;
}

LightParams Light::getParamsStructure()
{
	LightParams params{ };
	params.colour = glm::vec4(colour, strength);
	params.enabled = true;
	params.spot_angle = spot_angle;
	params.light_type = type;
	params.position = glm::vec4(transform.getPosition(), 0);
	params.direction = glm::normalize(transform.getMatrix() * glm::vec4{ 0, 0, -1, 0 });
	return params;
}

Light::Light(const LightType _type)
{
	type = _type;
	name = "light";
}
