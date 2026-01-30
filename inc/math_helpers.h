#pragma once

#include "transform.h"

namespace HopEngine
{

/**
 * @brief encapsulates a 3D bounding box defined by center and half-extent
 */
struct BoundingBox
{
    glm::vec3 center;
    glm::vec3 half_extent;
};

/**
 * @brief 3D ray-OBB intersection test.
 * @param ray_origin world space ray origin position.
 * @param ray_direction world space ray origin direction.
 * @param bounding_box bounding box described in the local space of the transform.
 * @param transform information which transforms the bounding box into world space.
 * @return closest distance (t) to the OBB if there was a hit; 0 if the ray origin
 * was inside the OBB; or INFINITY if there was no intersection.
 */
float intersect(glm::vec3 ray_origin, glm::vec3 ray_direction, const BoundingBox& bounding_box, Transform& transform);

}