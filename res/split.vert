#version 450

#define VERTEX
#include "common.glsl"

void main()
{
    frag.position = (object.model_to_world * vec4(position, 1)).xyz;
    frag.colour = colour;
    frag.normal = normalize((object.model_to_world * vec4(normal, 0)).xyz);
    frag.tangent = normalize((object.model_to_world * vec4(tangent, 0)).xyz);
    frag.uv = uv;

    gl_Position = scene.view_to_clip * scene.world_to_view * object.model_to_world * vec4(position.xyz, 1.0);
}