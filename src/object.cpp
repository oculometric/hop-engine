#include "object.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "mesh.h"
#include "material.h"
#include "uniform_block.h"
#include "graphics_environment.h"

using namespace HopEngine;
using namespace std;

struct ObjectUniforms
{
	glm::mat4 model_to_world;
	int id;
};

Object::Object(Ref<Mesh> _mesh, Ref<Material> _material)
{
	transform = Transform(this);
	mesh = _mesh;
	material = _material;
	uniforms = new UniformBlock(ShaderLayout{ GraphicsEnvironment::get()->getObjectDescriptorSetLayout(), {{ 0, UNIFORM, sizeof(ObjectUniforms) }} });
	
	DBG_VERBOSE("created object");
}

void Object::pushToDescriptorSet(size_t index)
{
	ObjectUniforms* object_uniforms = (ObjectUniforms*)(uniforms->getBuffer());

	object_uniforms->id = (int)(size_t)this;
	object_uniforms->model_to_world = transform.getMatrix();

	uniforms->pushToDescriptorSet(index);
}

VkDescriptorSet Object::getDescriptorSet(size_t index)
{
	return uniforms->getDescriptorSet(index);
}

Object::~Object()
{
	DBG_VERBOSE("destroying object " + PTR(this));
}

struct SceneUniforms
{
	glm::mat4 world_to_view;
	glm::mat4 view_to_clip;
	glm::ivec2 viewport_size;
	float time;
	float _pad3;
	glm::vec3 eye_position;
	float _pad1;
	glm::vec2 near_far;
};

Camera::Camera()
{
	uniforms = new UniformBlock(ShaderLayout{ GraphicsEnvironment::get()->getSceneDescriptorSetLayout(), {{ 0, UNIFORM, sizeof(SceneUniforms) }} });
	DBG_VERBOSE("created camera");
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
	scene_uniforms.near_far = { near_clip, far_clip };

	memcpy(uniforms->getBuffer(), &scene_uniforms, sizeof(SceneUniforms));
	uniforms->pushToDescriptorSet(index);
}

VkDescriptorSet Camera::getDescriptorSet(size_t index)
{
	return uniforms->getDescriptorSet(index);
}

Camera::~Camera()
{
	DBG_VERBOSE("destroying camera " + PTR(this));
}
