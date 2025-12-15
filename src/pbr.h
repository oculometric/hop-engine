#pragma once

#include <glm/vec4.hpp>
#include <glm/vec3.hpp>

struct LightParams
{
    glm::vec4 position = { 2, 0, 2, 0 };
    glm::vec4 direction = { -1, 0, -1, 0 };
    glm::vec4 colour = { 1, 0, 0, 0 };
    float spot_angle = 0.0f;
    int light_type = 0;
    bool enabled = false;
    float padding;
};

struct MaterialParams
{
    glm::vec4 diffuse = { 1, 1, 1, 0 };
    glm::vec4 specular = { 1, 1, 1, 0 };
    glm::vec4 emissive = { 0, 0, 0, 0 };
    float specular_exponent = 32.0f;
    glm::vec3 padding;
};
