#include "object.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "mesh.h"
#include "material.h"
#include "uniform_block.h"
#include "graphics_environment.h"
#include "pbr.h"

using namespace HopEngine;
using namespace std;

Object::Object()
{
	transform = Transform();
	uniforms = new UniformBlock(ShaderLayout{ RenderServer::getObjectDescriptorSetLayout(), {{ 0, UNIFORM, sizeof(ObjectUniforms) }} });
	name = "object";

	DBG_VERBOSE("created object");
}

void Object::_setParent(Ref<Object> new_parent)
{
	glm::mat4 world_transform = transform.getMatrix();
	parent = new_parent;
	if (parent)
		transform.parent_transform = &parent->transform;
	else
		transform.parent_transform = nullptr;
	transform.setMatrix(world_transform);
}

Ref<Object> Object::getParent()
{
	return parent;
}

void Object::pushToDescriptorSet(size_t index)
{
	ObjectUniforms* object_uniforms = (ObjectUniforms*)(uniforms->getBuffer());

	object_uniforms->id = (int)(size_t)this;
	object_uniforms->model_to_world = transform.getMatrix();

	uniforms->pushToDescriptorSet(index);
}

vector<DrawCommand> Object::getDrawCommands() const
{
	return { };
	//return { { RenderServer::getGizmoMaterial(), RenderServer::getGizmoMesh(0), uniforms } };
}

Object::~Object()
{
	DBG_VERBOSE("destroying object " + PTR(this));
}

Camera::Camera() : Object()
{
	uniforms = new UniformBlock(ShaderLayout{ RenderServer::getSceneDescriptorSetLayout(), {{ 0, UNIFORM, sizeof(SceneUniforms) }} });
	name = "camera";
}

void Camera::pushToDescriptorSet(size_t index) { }

void Camera::pushToCameraDescriptorSet(size_t index, glm::ivec2 viewport_size, float time, vector<LightParams> lights, glm::vec4 ambient)
{
	SceneUniforms scene_uniforms = getSceneUniforms(viewport_size, time, lights, ambient);

	memcpy(uniforms->getBuffer(), &scene_uniforms, sizeof(SceneUniforms));
	uniforms->pushToDescriptorSet(index);
}

SceneUniforms Camera::getSceneUniforms(glm::ivec2 viewport_size, float time, std::vector<LightParams> lights, glm::vec4 ambient)
{
	SceneUniforms scene_uniforms;
	scene_uniforms.time = time;
	scene_uniforms.eye_position = transform.getPosition();
	scene_uniforms.viewport_size = viewport_size;
	scene_uniforms.world_to_view = glm::inverse(transform.getMatrix());
	scene_uniforms.view_to_clip = glm::perspective(glm::radians(fov), viewport_size.x / (float)(viewport_size.y), near_clip, far_clip);
	scene_uniforms.view_to_clip[1][1] *= -1;
	scene_uniforms.clip_to_view = glm::inverse(scene_uniforms.view_to_clip);
	scene_uniforms.near_far = { near_clip, far_clip };
	memcpy(scene_uniforms.lights, lights.data(), lights.size() * sizeof(LightParams));
	scene_uniforms.ambient_light = ambient;

	return scene_uniforms;
}

glm::mat4 Camera::getWorldToScreenMatrix()
{
	glm::vec2 viewport_size = RenderServer::getFramebufferSize();
	glm::mat4 view_to_clip = glm::perspective(glm::radians(fov), viewport_size.x / (float)(viewport_size.y), near_clip, far_clip);
	view_to_clip[1][1] *= -1;
	glm::mat4 world_to_view = glm::inverse(transform.getMatrix());
	world_to_view[0] = glm::normalize(world_to_view[0]);
	world_to_view[1] = glm::normalize(world_to_view[1]);
	world_to_view[2] = glm::normalize(world_to_view[2]);
    return view_to_clip * world_to_view;
}

VkDescriptorSet Camera::getDescriptorSet(size_t index) const
{
	return uniforms->getDescriptorSet(index);
}

StaticMesh::StaticMesh(Ref<Mesh> _mesh, Ref<Material> _material) : Object()
{
	mesh = _mesh;
	material = _material;
	name = "static mesh";
}

void StaticMesh::pushToDescriptorSet(size_t index)
{
	Object::pushToDescriptorSet(index);
	material->pushToDescriptorSet(index);
}

vector<DrawCommand> StaticMesh::getDrawCommands() const
{
	vector<DrawCommand> commands;
	if (material && mesh && uniforms)
		commands.push_back(DrawCommand(material, mesh, uniforms));
	return commands;
}

Light::Light(LightType _type)
{
	type = _type;
	name = "light";
}

LightParams Light::getParamsStructure() const
{
	LightParams params{ };
	params.colour = colour;
	params.enabled = true;
	params.spot_angle = spot_angle;
	params.light_type = type;
	params.position = glm::vec4(transform.getPosition(), 0);
	params.direction = glm::normalize(transform.getMatrix() * glm::vec4{ 0, 0, -1, 0 });
	return params;
}
