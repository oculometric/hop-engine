#version 450

#define VERTEX
#include "common.glsl"

void main()
{
    frag.position = vec4(in_position.xyz, 1);
    frag.colour = in_colour;
    frag.normal = in_normal;
    frag.tangent = in_tangent;
    frag.uv = in_uv;
    gl_Position = frag.position;
}