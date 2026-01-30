#pragma once

#include <glm/glm.hpp>

/**
 * @brief describes a 3D scene light. see the \code Light\endcode class.
 */
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

/**
 * @brief structure which mirrors the standard object uniform
 * buffer (i.e. descriptor set 1).
 */
struct ObjectUniforms
{
    glm::mat4 model_to_world;
    int id;
};

/**
 * @brief structure which mirrors the standard scene uniform
 * buffer (i.e. descriptor set 0).
 */
struct SceneUniforms
{
    glm::mat4 world_to_view;
    glm::mat4 view_to_clip;
    glm::mat4 clip_to_view;
    glm::mat4 view_to_world;
    glm::ivec2 viewport_size = { 0, 0 };
    glm::vec2 padding = { 0, 0 };
    glm::vec3 eye_position = { 0, 0, 0 };
    float time = 0;
    glm::vec2 near_far = { 0, 0 };
    glm::vec2 padding2 = { 0, 0 };
    LightParams lights[8];
    glm::vec4 ambient_light = { 0, 0.05f, 0.05f, 0 };
};
