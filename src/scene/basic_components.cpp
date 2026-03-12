#include "basic_components.h"

#include <glm/gtc/matrix_transform.hpp>

#include "mesh.h"
#include "material.h"
#include "uniform_block.h"
#include "render_server.h"
#include "command_buffer.h"
#include "scene.h"
#include "engine.h"

using namespace HopEngine;
using namespace std;


/*


void Object::updateObjectUniforms()
{
	ObjectUniforms* object_uniforms = static_cast<ObjectUniforms*>(uniforms->getBuffer());

	object_uniforms->id = static_cast<int>(reinterpret_cast<size_t>(this));
	object_uniforms->model_to_world = transform.getMatrix();
}

Ref<CameraComponent> CameraComponent::create()
{
	Ref obj = new CameraComponent();
	obj->self = obj.cast<Object>();
	return obj;
}

void CameraComponent::bind(const Ref<DrawCommandBuffer>& command_buffer, const glm::ivec2 viewport_size, const vector<LightParams>& lights, const glm::vec4 ambient)
{
	SceneUniforms scene_uniforms = getSceneUniforms(viewport_size, lights, ambient);

	memcpy(uniforms->getBuffer(), &scene_uniforms, sizeof(SceneUniforms));
	uniforms->bind(command_buffer, 0);
}

SceneUniforms CameraComponent::getSceneUniforms(const glm::ivec2 viewport_size, const vector<LightParams>& lights, const glm::vec4 ambient)
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

glm::mat4 CameraComponent::getWorldToScreenMatrix()
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

CameraComponent::CameraComponent() : Object()
{
	uniforms = RenderServer::createSceneUniforms();
	name = "camera";
}

Ref<StaticMeshComponent> StaticMeshComponent::create(const Ref<Mesh>& _mesh, const Ref<Material>& _material)
{
	Ref obj = new StaticMeshComponent(_mesh, _material);
	obj->self = obj.cast<Object>();
	return obj;
}

vector<DrawCommand> StaticMeshComponent::getDrawCommands()
{
	updateObjectUniforms();
	vector<DrawCommand> commands;
	if (material && mesh && uniforms)
		commands.push_back(DrawCommand(material, mesh, uniforms).mask(camera_mask));
	return commands;
}

BoundingBox StaticMeshComponent::getLocalBounds() const
{
	return mesh->getBoundingBox();
}

StaticMeshComponent::StaticMeshComponent(const Ref<Mesh>& _mesh, const Ref<Material>& _material) : Object()
{
	mesh = _mesh;
	material = _material;
	name = "static mesh";
}

Ref<LightComponent> LightComponent::create(LightType _type)
{
	Ref obj = new LightComponent(_type);
	obj->self = obj.cast<Object>();
	return obj;
}

LightParams LightComponent::getParamsStructure()
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

LightComponent::LightComponent(const LightType _type)
{
	type = _type;
	name = "light";
}
*/