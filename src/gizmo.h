#pragma once

#include <glm/vec3.hpp>

#include "common.h"
#include "object.h"

namespace HopEngine
{

class Gizmo : public StaticMesh
{
private:
    glm::vec3 current_colour;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Gizmo);

    Gizmo();

    void trackObject(WeakRef<Object> object, WeakRef<Camera> camera);

private:
    void setHighlightColour(glm::vec3 new_colour);
};

}