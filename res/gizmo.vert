#version 450

#define VERTEX
#include "common.glsl"

void main()
{
    frag.position = (object.model_to_world * vec4(position.xyz, 1));
    frag.colour = colour;

    gl_Position = scene.view_to_clip * scene.world_to_view * object.model_to_world * vec4(position.xyz, 1.0);
}