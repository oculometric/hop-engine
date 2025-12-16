#version 450

#define VERTEX
#include "common.glsl"

void main()
{
    frag.position = (object.model_to_world * vec4(in_position.xyz, 1));
    frag.colour = in_colour;

    vec4 origin = scene.world_to_view * object.model_to_world * vec4(0, 0, 0, 1);
    float depth = length(origin.z / origin.w) * 0.1f;

    mat4 to_clip = scene.view_to_clip * scene.world_to_view * object.model_to_world * mat4(depth, 0, 0, 0, 0, depth, 0, 0, 0, 0, depth, 0, 0, 0, 0, 1);

    gl_Position = to_clip * vec4(in_position.xyz, 1.0);
}