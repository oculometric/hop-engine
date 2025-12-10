#version 450

#define VERTEX
#include "common.glsl"

void main()
{
    frag.position = vec4(position.xyz, 1);
    frag.colour = colour;
    frag.normal = normal;
    frag.tangent = tangent;
    frag.uv = uv;
    gl_Position = frag.position;
}