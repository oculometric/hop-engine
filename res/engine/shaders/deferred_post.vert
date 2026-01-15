#version 450

#define VERTEX
#include "common.glsl"

void main()
{
    frag.uv = in_uv;
    frag.position = vec4(in_position.xyz, 1);
    gl_Position = vec4(in_position.xyz, 1);
}