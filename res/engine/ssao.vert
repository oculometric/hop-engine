#version 450

#define VERTEX
#include "common.glsl"

void main()
{
    frag.position = vec4(in_position.xyz, 1);
    frag.uv = in_uv;
    gl_Position = vec4(in_position.xyz, 1);
}