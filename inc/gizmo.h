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
    static Ref<Gizmo> create();

    void trackObject(const WeakRef<Object>& object, const WeakRef<Camera>& camera);

protected:
    Gizmo();
    
private:
    void setHighlightColour(glm::vec3 new_colour);
};

}