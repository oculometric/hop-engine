#version 450

#define VERTEX
#include "common.glsl"

void main()
{
    frag.position = (object.model_to_world * vec4(in_position.xyz, 1));
    frag.colour = in_colour;
    frag.uv = in_uv;
    gl_Position = scene.view_to_clip * scene.world_to_view * frag.position;
}