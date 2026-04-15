#include "basic_components.h"

#include "command_buffer.h"
#include "engine.h"
#include "material.h"
#include "mesh.h"
#include "render_server.h"
#include "scene.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace HopEngine;

void CameraComponent::awake()
{
    uniforms = RenderServer::createSceneUniforms();
    object_uniforms = RenderServer::createObjectUniforms();
}

WeakRef<UniformBlock> CameraComponent::getUniforms(glm::ivec2 viewport_size,
    const std::vector<LightParams>& lights, glm::vec4 ambient)
{
    SceneUniforms* scene_uniforms = reinterpret_cast<SceneUniforms*>(uniforms->getBuffer());
    scene_uniforms->time          = Engine::getEngineTime();
    scene_uniforms->eye_position  = getTransform().getPosition();
    scene_uniforms->viewport_size = viewport_size;
    scene_uniforms->world_to_view = glm::inverse(getTransform().getMatrix());
    scene_uniforms->view_to_clip  = glm::perspective(glm::radians(fov),
        static_cast<float>(viewport_size.x) / static_cast<float>(viewport_size.y), near_clip, far_clip);
    scene_uniforms->view_to_clip[1][1] *= -1;
    scene_uniforms->clip_to_view  = glm::inverse(scene_uniforms->view_to_clip);
    scene_uniforms->view_to_world = getTransform().getMatrix();
    scene_uniforms->near_far      = { near_clip, far_clip };
    memcpy(scene_uniforms->lights, lights.data(), lights.size() * sizeof(LightParams));
    scene_uniforms->ambient_light = ambient;

    return uniforms;
}

glm::mat4 CameraComponent::getWorldToScreenMatrix()
{
    const glm::vec2 viewport_size = RenderServer::getFramebufferSize();
    glm::mat4 view_to_clip        = glm::perspective(glm::radians(fov),
        viewport_size.x / static_cast<float>(viewport_size.y), near_clip, far_clip);
    view_to_clip[1][1] *= -1;
    glm::mat4 world_to_view = glm::inverse(getTransform().getMatrix());
    world_to_view[0]        = glm::normalize(world_to_view[0]);
    world_to_view[1]        = glm::normalize(world_to_view[1]);
    world_to_view[2]        = glm::normalize(world_to_view[2]);
    return view_to_clip * world_to_view;
}

std::vector<DrawCommand> CameraComponent::getDrawCommands()
{
    ObjectUniforms* u = reinterpret_cast<ObjectUniforms*>(object_uniforms->getBuffer());
    u->id             = static_cast<int>(reinterpret_cast<size_t>(this));
    u->model_to_world = getTransform().getMatrix();

    return { DrawCommand(Engine::loadMaterial("res://engine/materials/camera_gizmo.hmat"), RenderServer::getQuad(), object_uniforms).priority(-1000) };
}

void StaticMeshComponent::awake() { uniforms = RenderServer::createObjectUniforms(); }

std::vector<DrawCommand> StaticMeshComponent::getDrawCommands()
{
    if (!uniforms)
    {
        DBG_WARNING("static mesh component had no uniforms! creating them now...");
        uniforms = RenderServer::createObjectUniforms();
    }
    ObjectUniforms* object_uniforms = reinterpret_cast<ObjectUniforms*>(uniforms->getBuffer());
    object_uniforms->id             = static_cast<int>(reinterpret_cast<size_t>(this));
    object_uniforms->model_to_world = getTransform().getMatrix();

    return { DrawCommand(material, mesh, uniforms).mask(camera_mask) };
}

BoundingBox StaticMeshComponent::getLocalBounds() const { return mesh->getBoundingBox(); }

LightParams LightComponent::getParamsStructure() const
{
    LightParams params{};
    params.colour     = glm::vec4(colour, strength);
    params.enabled    = true;
    params.spot_angle = spot_angle;
    params.light_type = type;
    params.position   = glm::vec4(getTransform().getPosition(), 0);
    params.direction  = glm::normalize(getTransform().getMatrix() * glm::vec4{ 0, 0, -1, 0 });
    return params;
}

void LightComponent::awake()
{
    object_uniforms = RenderServer::createObjectUniforms();
}

std::vector<DrawCommand> LightComponent::getDrawCommands()
{
    ObjectUniforms* u = reinterpret_cast<ObjectUniforms*>(object_uniforms->getBuffer());
    u->id             = static_cast<int>(reinterpret_cast<size_t>(this));
    u->model_to_world = getTransform().getMatrix();

    return { DrawCommand(Engine::loadMaterial("res://engine/materials/light_gizmo.hmat"), RenderServer::getQuad(), object_uniforms).priority(-1000) };
}
