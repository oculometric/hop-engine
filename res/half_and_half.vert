#version 450

#define VERTEX
#include "engine/common.glsl"

void main()
{
    frag.uv = in_uv;
    gl_Position = vec4(in_position.xyz, 1);
}