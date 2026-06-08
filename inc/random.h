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
#include "glm/vec3.hpp"

namespace HopEngine
{

class Random
{
public:
    /**
     * @brief generates a random float between `min` and `max` inclusive.
     * @param min lower bound for the range.
     * @param max upper bound for the range.
     * @returns random float within the range.
     */
    static float range(float min, float max);
    /**
     * @brief generates a random float between `0` and `1` inclusive.
     * @returns random float.
     */
    static float clamped() { return range(0.0f, 1.0f); }
    /**
     * @brief generates a random float based on a normal distribution with the specified mean and standard
     * deviation.
     * @param mean mean value for the normal distribution function.
     * @param deviation standard deviation from the mean, for the normal distribution function.
     * @returns random float from the distribution.
     */
    static float normalDistribution(float mean, float deviation);
    /**
     * @brief generates a random integer between `min` and `max` inclusive.
     * @param min lower bound for the range.
     * @param max upper bound for the range.
     * @returns random integer within the range.
     */
    static int range(int min, int max) { return limit((max - min) + 1) + min; }
    /**
     * @brief generates a random unsigned integer less than `limit` (and greater than or equal to zero).
     * useful if you want a random index into an array.
     * @param limit upper bound for the range (exclusive).
     * @returns random integer below the limit.
     */
    static size_t limit(size_t limit);
    /**
     * @brief generates a random vector within 90 degrees of the specified `vector`. cosine weighted, result
     * is the same length as the input.
     * @param vector vector around which the hemisphere is oriented.
     * @returns random vector within the hemisphere.
     */
    static glm::vec3 hemisphere(glm::vec3 vector);
    /**
     * @brief generates a random normalised vector.
     * @returns random vector.
     */
    static glm::vec3 normal();
    /**
     * @brief queries the next random number from the internal generator.
     * @returns random size_t.
     */
    static size_t next();

    /**
     * @brief scrambles the seed of the internal random number generator.
     */
    static void randomiseSeed();
    /**
     * @brief assigns a specific seed for the internal random number generator.
     */
    static void setSeed(size_t seed);
};

} // namespace HopEngine