#version 450

#define FRAGMENT
#include "common.glsl"

layout(set = 2, binding = 0) uniform sampler2D tex;

void main()
{
    out_colour = vec4(toSRGB(texture(tex, frag.uv).rgb), 1);
}