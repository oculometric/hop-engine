#version 450

#define FRAGMENT
#define NONSTANDARD_FRAG_OUT
#define OMIT_OBJECT_SET
#include "../engine/shaders/common.glsl"

layout(location = 0) out vec4 out_colour;

layout(set = 2, binding = 0) uniform sampler2D original;
layout(set = 2, binding = 1) uniform sampler2D multiply;

void main()
{
    out_colour = vec4(texture(original, frag.uv).rgb * texture(multiply, frag.uv).rgb, 1);
}