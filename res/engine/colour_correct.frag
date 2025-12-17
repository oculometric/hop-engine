#version 450

#define FRAGMENT
#define NONSTANDARD_FRAG_OUT
#define OMIT_OBJECT_SET
#include "common.glsl"

layout(location = 0) out vec4 out_colour;

layout(set = 2, binding = 0) uniform sampler2D tex;

layout(set = 2, binding = 2) uniform Params
{
    float gamma;
    float exposure;
    float offset;
};

vec3 gammaAdjust(vec3 value)
{
    return (exposure * pow(value + offset, vec3(gamma)));
}

void main()
{
    out_colour = vec4(gammaAdjust(texture(tex, frag.uv).rgb), 1);
}