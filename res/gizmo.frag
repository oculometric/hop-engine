#version 450

#define NONSTANDARD_FRAG_OUT
#define FRAGMENT
#include "common.glsl"

layout(location = 0) out vec4 colour;

void main()
{
    colour = vec4(frag.colour.rgb, 1);
}