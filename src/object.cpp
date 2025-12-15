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

struct ObjectUniforms
{
	glm::mat4 model_to_world;
	int id;
};

Object::Object()
{
	transform = Transform();
	uniforms = new UniformBlock(ShaderLayout{ RenderServer::getObjectDescriptorSetLayout(), {{ 0, UNIFORM, sizeof(ObjectUniforms) }} });
	
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

void Object::pushToDescriptorSet(size_t index)
{
	ObjectUniforms* object_uniforms = (ObjectUniforms*)(uniforms->getBuffer());

	object_uniforms->id = (int)(size_t)this;
	object_uniforms->model_to_world = transform.getMatrix();

	uniforms->pushToDescriptorSet(index);
}

vector<DrawCommand> Object::getDrawCommands() const
{
	return { { RenderServer::getGizmoMaterial(), RenderServer::getGizmoMesh(0), uniforms } };
}

Object::~Object()
{
	DBG_VERBOSE("destroying object " + PTR(this));
}

struct SceneUniforms
{
	glm::mat4 world_to_view;
	glm::mat4 view_to_clip;
	glm::mat4 clip_to_view;
	glm::ivec2 viewport_size;
	glm::vec2 padding;
	glm::vec3 eye_position;
	float time;
	glm::vec2 near_far;
	glm::vec2 padding2;
	LightParams lights[8];
	glm::vec4 ambient_light = { 0, 0.05f, 0.05f, 0 };
};

Camera::Camera() : Object()
{
	uniforms = new UniformBlock(ShaderLayout{ RenderServer::getSceneDescriptorSetLayout(), {{ 0, UNIFORM, sizeof(SceneUniforms) }} });
}

void Camera::pushToDescriptorSet(size_t index, glm::ivec2 viewport_size, float time)
{
	SceneUniforms scene_uniforms;
	scene_uniforms.time = time;
	scene_uniforms.eye_position = transform.getLocalPosition();
	scene_uniforms.viewport_size = viewport_size;
	scene_uniforms.world_to_view = glm::inverse(transform.getMatrix());
	scene_uniforms.view_to_clip = glm::perspective(glm::radians(fov), viewport_size.x / (float)(viewport_size.y), near_clip, far_clip);
	scene_uniforms.view_to_clip[1][1] *= -1;
	scene_uniforms.clip_to_view = glm::inverse(scene_uniforms.view_to_clip);
	scene_uniforms.near_far = { near_clip, far_clip };
	scene_uniforms.lights[0] = LightParams{ { 2, 0, 2, 0 } };

	memcpy(uniforms->getBuffer(), &scene_uniforms, sizeof(SceneUniforms));
	uniforms->pushToDescriptorSet(index);
}

VkDescriptorSet Camera::getDescriptorSet(size_t index) const
{
	return uniforms->getDescriptorSet(index);
}

StaticMesh::StaticMesh(Ref<Mesh> _mesh, Ref<Material> _material) : Object()
{
	mesh = _mesh;
	material = _material;
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
		commands.push_back({ material, mesh, uniforms });
	commands.push_back(Object::getDrawCommands()[0]);
	return commands;
}
