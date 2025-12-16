#version 450

#define FRAGMENT
#define NONSTANDARD_FRAG_OUT
#define OMIT_OBJECT_SET
#include "common.glsl"

layout(location = 0) out vec4 out_colour;

layout(set = 2, binding = 0) uniform sampler2D screen_texture;

void main()
{
    out_colour = vec4(texture(screen_texture, frag.uv).rgb, 1);
}