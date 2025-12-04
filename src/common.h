#pragma once

#define DELETE_CONSTRUCTORS(name) name##() = delete;\
    name##(const name##& other) = delete;\
    name##(name##&& other) = delete;\
    name##& operator=(const name##& other) = delete;\
    name##& operator=(name##&& other) = delete

#include "counted_ref.h"

#include <glm/common.hpp>

struct SceneUniforms
{
    glm::mat4 world_to_view;
    glm::mat4 view_to_clip;
    float time;
};

struct ObjectUniforms
{
    glm::mat4 model_to_world;
    int id;
};
