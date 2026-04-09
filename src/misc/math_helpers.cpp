#include "math_helpers.h"

using namespace HopEngine;
using namespace std;

float HopEngine::intersect(glm::vec3 ray_origin, glm::vec3 ray_direction,
    const BoundingBox& bounding_box, glm::mat4 transform)
{
    // transform ray into bounding box space
    glm::mat4 world_to_model      = glm::inverse(transform);
    glm::vec3 local_ray_origin    = world_to_model * glm::vec4(ray_origin, 1.0f);
    glm::vec3 local_ray_direction = world_to_model * glm::vec4(glm::normalize(ray_direction), 0.0f);

    // compute max and min bounding box corners
    glm::vec3 min = bounding_box.center - bounding_box.half_extent;
    glm::vec3 max = bounding_box.center + bounding_box.half_extent;

    // test if ray origin is inside the bounding box (in which case return 0)
    if (local_ray_origin.x < max.x && local_ray_origin.y < max.y && local_ray_origin.z < max.z &&
        local_ray_origin.x > min.x && local_ray_origin.y > min.y && local_ray_origin.z > min.z)
        return 0.0f;

    // based on this:
    // https://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-custom-ray-obb-function/
    glm::vec3 delta = bounding_box.center - local_ray_origin;
    glm::vec3 t1s   = (delta + min) / local_ray_direction;
    glm::vec3 t2s   = (delta + max) / local_ray_direction;

    float t_min = 0.0f;
    float t_max = INFINITY;

    // intersection checks
    float t1 = t1s.x;
    float t2 = t2s.x;
    if (t1 > t2)
    {
        t1 = t2s.x;
        t2 = t1s.x;
    }
    t_max = std::min(t2, t_max);
    t_min = std::max(t1, t_min);

    t1 = t1s.y;
    t2 = t2s.y;
    if (t1 > t2)
    {
        t1 = t2s.y;
        t2 = t1s.y;
    }
    t_max = std::min(t2, t_max);
    t_min = std::max(t1, t_min);

    t1 = t1s.z;
    t2 = t2s.z;
    if (t1 > t2)
    {
        t1 = t2s.z;
        t2 = t1s.z;
    }
    t_max = std::min(t2, t_max);
    t_min = std::max(t1, t_min);

    if (t_max < t_min) return INFINITY;

    return t_min;
}