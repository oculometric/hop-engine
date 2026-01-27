#pragma once

#include "transform.h"

namespace HopEngine
{

struct BoundingBox
{
    glm::vec3 center;
    glm::vec3 half_extent;
};

float intersect(glm::vec3 ray_origin, glm::vec3 ray_direction, const BoundingBox& bounding_box, Transform& transform);

}