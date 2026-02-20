#include "object.h"

#include <glm/gtc/matrix_transform.hpp>

#include "mesh.h"
#include "material.h"
#include "uniform_block.h"
#include "render_server.h"
#include "pbr.h"
#include "command_buffer.h"

using namespace HopEngine;
using namespace std;

Object::Object()
{
	transform = Transform();
	uniforms = new UniformBlock(ShaderLayout{ RenderServer::getObjectDescriptorSetLayout(), {{ 0, UNIFORM, sizeof(ObjectUniforms) }} });
	name = "object";

	DBG_VERBOSE("created object");
}

Object::~Object()
{
	DBG_VERBOSE("destroying object '" + name + "' (" + PTR(this) + ')');
}

vector<DrawCommand> Object::getDrawCommands() const
{
	return { };
}

Ref<Object> Object::getParent()
{
	return parent;
}

BoundingBox Object::getLocalBounds() const
{
	return BoundingBox{ { 0, 0, 0 }, { 0.25f, 0.25f, 0.25f } };
}

void Object::pushToDescriptorSet(const size_t index)
{
	ObjectUniforms* object_uniforms = static_cast<ObjectUniforms*>(uniforms->getBuffer());

	object_uniforms->id = static_cast<int>(reinterpret_cast<size_t>(this));
	object_uniforms->model_to_world = transform.getMatrix();

	uniforms->pushToDescriptorSet(index);
}

void Object::_setParent(const Ref<Object>& new_parent)
{
	const glm::mat4 world_transform = transform.getMatrix();
	parent = new_parent;
	if (parent)
		transform.parent_transform = &parent->transform;
	else
		transform.parent_transform = nullptr;
	transform.setMatrix(world_transform);
}

Camera::Camera() : Object()
{
	uniforms = new UniformBlock(ShaderLayout{ RenderServer::getSceneDescriptorSetLayout(), {{ 0, UNIFORM, sizeof(SceneUniforms) }} });
	name = "camera";
}

void Camera::bind(Ref<DrawCommandBuffer> command_buffer)
{
	uniforms->bind(command_buffer, 0);
}

SceneUniforms Camera::getSceneUniforms(const glm::ivec2 viewport_size, const float time, const vector<LightParams>& lights, const glm::vec4 ambient)
{
	SceneUniforms scene_uniforms;
	scene_uniforms.time = time;
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

void Camera::pushToDescriptorSet(size_t index) { }

void Camera::pushToCameraDescriptorSet(const size_t index, const glm::ivec2 viewport_size, const float time, const vector<LightParams>& lights, const glm::vec4 ambient)
{
	SceneUniforms scene_uniforms = getSceneUniforms(viewport_size, time, lights, ambient);

	memcpy(uniforms->getBuffer(), &scene_uniforms, sizeof(SceneUniforms));
	uniforms->pushToDescriptorSet(index);
}

StaticMesh::StaticMesh(const Ref<Mesh>& _mesh, const Ref<Material>& _material) : Object()
{
	mesh = _mesh;
	material = _material;
	name = "static mesh";
}

vector<DrawCommand> StaticMesh::getDrawCommands() const
{
	vector<DrawCommand> commands;
	if (material && mesh && uniforms)
		commands.push_back(DrawCommand(material, mesh, uniforms).mask(camera_mask));
	return commands;
}

BoundingBox StaticMesh::getLocalBounds() const
{
	return mesh->getBoundingBox();
}

void StaticMesh::pushToDescriptorSet(const size_t index)
{
	Object::pushToDescriptorSet(index);
	if (material)
		material->pushToDescriptorSet(index);
}

Light::Light(const LightType _type)
{
	type = _type;
	name = "light";
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
