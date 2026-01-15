#version 450

#define NONSTANDARD_FRAG_OUT
#define FRAGMENT
#include "common.glsl"

layout(location = 0) out vec4 out_colour;

layout(set = 2, binding = 0) uniform MaterialUniforms
{
    vec3 colour_filter;
};

void main()
{
    if (length(frag.colour.rgb - colour_filter) < 0.01f)
        out_colour = vec4(1);
    else
        out_colour = vec4(frag.colour.rgb, 1);
    out_params.w = 0.0f;
}