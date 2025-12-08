#version 450

#define VERTEX
#include "common.glsl"

void main()
{
    frag.position = (object.model_to_world * vec4(position, 1)).xyz;
    frag.colour = colour;
    frag.normal = normal.xyz;
    frag.tangent = normal.xyz;
    frag.uv = uv;

    gl_Position = scene.view_to_clip * scene.world_to_view * object.model_to_world * vec4(position.xyz, 1);
}