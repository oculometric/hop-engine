#version 450

#define VERTEX
#include "common.glsl"

void main()
{
    frag.position = (object.model_to_world * vec4(position.xyz, 1));
    frag.colour = colour;
    frag.normal = vec4(normalize((object.model_to_world * vec4(normal.xyz, 0)).xyz), 0);
    frag.tangent = vec4(normalize((object.model_to_world * vec4(tangent.xyz, 0)).xyz), 0);
    frag.uv = uv;

    gl_Position = scene.view_to_clip * scene.world_to_view * object.model_to_world * vec4(position.xyz, 1.0);
}