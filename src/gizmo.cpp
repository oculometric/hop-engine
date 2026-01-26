#include "gizmo.h"

#include "input.h"
#include "graphics_environment.h"
#include "material.h"
#include "mesh.h"

using namespace HopEngine;
using namespace std;

void Gizmo::trackObject(WeakRef<Object> object, WeakRef<Camera> camera)
{
    if (!object)
        return;

    if (Input::wasKeyPressed('Z'))
        mesh = RenderServer::getGizmoMesh(0);
    else if (Input::wasKeyPressed('X'))
        mesh = RenderServer::getGizmoMesh(1);
    else if (Input::wasKeyPressed('C'))
        mesh = RenderServer::getGizmoMesh(1);

    transform.setMatrix(object->transform.getMatrix());
    if (Input::isMouseDown(Input::MOUSE_LEFT))
    {
        glm::vec4 mouse_delta = glm::vec4(Input::getMouseDelta() / (RenderServer::getFramebufferSize() / 4.0f), 0, 0);
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
        glm::vec2 mouse_clip = ((Input::getMousePosition() / RenderServer::getFramebufferSize()) * 2.0f) - 1.0f;

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

Gizmo::Gizmo() : StaticMesh(RenderServer::getGizmoMesh(0), RenderServer::getGizmoMaterial()->duplicate())
{
    camera_mask = 0xF0000000;
    name = "gizmo";
}

void Gizmo::setHighlightColour(glm::vec3 new_colour)
{
    if (new_colour != current_colour)
        material->setVec3Uniform("colour_filter", new_colour);
    current_colour = new_colour;
}
