#version 450

#define FRAGMENT
#define NONSTANDARD_FRAG_OUT
#define OMIT_OBJECT_SET
#include "common.glsl"

layout(location = 0) out vec4 out_colour;

layout(set = 2, binding = 0) uniform sampler2D tex;
layout(set = 2, binding = 1) uniform sampler2D lut;

layout(set = 2, binding = 2) uniform Params
{
    float gamma;
    float exposure;
    float offset;
    float use_lut;
};

vec3 gammaAdjust(vec3 value)
{
    return (exposure * pow(value + offset, vec3(gamma)));
}

vec3 sampleLut(vec3 colour)
{
    float blue_chunk = floor(colour.b * 8.0f * 8.0f);
    vec2 uv_base = vec2(mod(blue_chunk, 8.0f) / 8.0f, floor(blue_chunk / 8.0f) / 8.0f);
    return toLinear(texture(lut, (colour.rg / 8.0f) + uv_base).bgr);
}

void main()
{
    if (use_lut > 0.5f)
        out_colour = vec4(sampleLut(texture(tex, frag.uv).rgb), 1);
    else
        out_colour = vec4(gammaAdjust(texture(tex, frag.uv).rgb), 1);
}