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

