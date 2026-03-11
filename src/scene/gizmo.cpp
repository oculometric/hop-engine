#include "gizmo.h"

#include "engine.h"
#include "input.h"
#include "render_server.h"
#include "material.h"
#include "mesh.h"
#include "scene.h"

using namespace HopEngine;
using namespace std;

Gizmo::Gizmo() : StaticMesh(nullptr, nullptr)
{
    mesh = Engine::loadMesh("res://engine/meshes/axes_gizmo.obj");
    material = new Material(Engine::loadShader("res://engine/shaders/gizmo.glsl"), Pipeline::Builder().cullMode(Pipeline::CULL_NONE));
    camera_mask = 0xFFFFFFFF;
    name = "gizmo";
}

Ref<Gizmo> Gizmo::create()
{
    Ref obj = new Gizmo();
    obj->self = obj.cast<Object>();
    return obj;
}

void Gizmo::trackObject(const WeakRef<Object>& object, const WeakRef<Camera>& camera)
{
    if (!object)
        return;

    if (Input::wasKeyPressed('Z'))
        mesh = Engine::loadMesh("res://engine/meshes/axes_gizmo.obj");
    else if (Input::wasKeyPressed('X'))
        mesh = Engine::loadMesh("res://engine/meshes/rotate_gizmo.obj");
    //else if (Input::wasKeyPressed('C'))
    //    mesh = RenderServer::getGizmoMesh(1);

    transform.setMatrix(object->transform.getMatrix());
    if (Input::isMouseDown(Input::MOUSE_LEFT))
    {
        glm::vec4 mouse_delta = glm::vec4(Input::getMouseDelta() / (glm::vec2(getScene()->getViewportSize()) / 4.0f), 0, 0);
        glm::mat4 screen_to_local = glm::inverse(camera->getWorldToScreenMatrix() * transform.getMatrix());
        glm::vec4 translation = screen_to_local * mouse_delta;
        object->transform.translateLocal(glm::vec3{ translation.x, translation.y, translation.z } * current_colour );
    }
    else
    {
        glm::mat4 gizmo_local_to_screen = camera->getWorldToScreenMatrix() * transform.getMatrix();
        glm::vec4 gizmo_x_arrow_pos = gizmo_local_to_screen * glm::vec4{ 1, 0, 0, 1 };
        gizmo_x_arrow_pos /= gizmo_x_arrow_pos.w;
        glm::vec4 gizmo_y_arrow_pos = gizmo_local_to_screen * glm::vec4{ 0, 1, 0, 1 };
        gizmo_y_arrow_pos /= gizmo_y_arrow_pos.w;
        glm::vec4 gizmo_z_arrow_pos = gizmo_local_to_screen * glm::vec4{ 0, 0, 1, 1 };
        gizmo_z_arrow_pos /= gizmo_z_arrow_pos.w;
        glm::vec2 mouse_clip = ((Input::getMousePosition() / glm::vec2(getScene()->getViewportSize())) * 2.0f) - 1.0f;

        float mouse_to_x_arrow = glm::length(mouse_clip - glm::vec2{ gizmo_x_arrow_pos.x, gizmo_x_arrow_pos.y});
        float mouse_to_y_arrow = glm::length(mouse_clip - glm::vec2{ gizmo_y_arrow_pos.x, gizmo_y_arrow_pos.y});
        float mouse_to_z_arrow = glm::length(mouse_clip - glm::vec2{ gizmo_z_arrow_pos.x, gizmo_z_arrow_pos.y});

        if (mouse_to_x_arrow < mouse_to_y_arrow)
        {
            if (mouse_to_x_arrow < mouse_to_z_arrow)
                setHighlightColour({ 1, 0, 0 });
            else
                setHighlightColour({ 0, 0, 1 });
        }
        else
        {
            if (mouse_to_y_arrow < mouse_to_z_arrow)
                setHighlightColour({ 0, 1, 0 });
            else
                setHighlightColour({ 0, 0, 1 });
        }
    }
}

void Gizmo::setHighlightColour(const glm::vec3 new_colour)
{
    if (new_colour != current_colour)
        material->setVec3Uniform("colour_filter", new_colour);
    current_colour = new_colour;
}
