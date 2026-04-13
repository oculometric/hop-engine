/*
 * HopEngine graphics engine toolkit.
 * Copyright (C) 2025  cassette costen

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "common.h"

#include <glm/glm.hpp>

namespace HopEngine
{

/**
 * @brief encapsulates a 3D bounding box defined by center and half-extent.
 */
struct BoundingBox final
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
 * @returns closest distance (t) to the OBB if there was a hit; 0 if the ray origin
 * was inside the OBB; or INFINITY if there was no intersection.
 */
float intersect(glm::vec3 ray_origin, glm::vec3 ray_direction, const BoundingBox& bounding_box,
    glm::mat4 transform);

/**
 * @brief encapsulates a spline, defined by an array of points.
 */
struct Spline
{
    std::vector<glm::vec3> points; // points defining the spline's shape
    bool loop = false;             // whether or not the spline interpolation should repeat

    /**
     * @brief operator which allows indexing/interpolating into spline via a fraction. uses Catmull-Romm
     * interpolation.
     * @param t 0-1 floating point value which interpolates along the points in the spline. values
     * outside the 0-1 range are either looped or clamped, depending on whether `loop` is enabled.
     * @returns 3D vector representing the position in space for the specified interpolation fraction.
     */
    glm::vec3 operator[](float t) const;
};

} // namespace HopEngine