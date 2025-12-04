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
	mesh = _mesh;
	material = _material;
	uniforms = new UniformBlock(GraphicsEnvironment::get()->getObjectDescriptorSetLayout(), sizeof(ObjectUniforms));
}

Ref<Mesh> Object::getMesh()
{
	return mesh;
}

Ref<Material> Object::getMaterial()
{
	return material;
}

void Object::pushToDescriptorSet(size_t index)
{
	ObjectUniforms* object_uniforms = (ObjectUniforms*)(uniforms->getBuffer());

	object_uniforms->id = (int)(size_t)this;
	object_uniforms->model_to_world = glm::mat4(1);
	object_uniforms->model_to_world = glm::scale(object_uniforms->model_to_world, scale);
	object_uniforms->model_to_world = glm::rotate(object_uniforms->model_to_world, rotation.x, glm::vec3(1, 0, 0));
	object_uniforms->model_to_world = glm::rotate(object_uniforms->model_to_world, rotation.y, glm::vec3(0, 1, 0));
	object_uniforms->model_to_world = glm::rotate(object_uniforms->model_to_world, rotation.z, glm::vec3(0, 0, 1));
	object_uniforms->model_to_world = glm::translate(object_uniforms->model_to_world, position);

	uniforms->pushToDescriptorSet(index);
}

VkDescriptorSet Object::getDescriptorSet(size_t index)
{
	return uniforms->getDescriptorSet(index);
}

