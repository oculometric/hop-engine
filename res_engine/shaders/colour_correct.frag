#version 450

#define FRAGMENT
#define NONSTANDARD_FRAG_OUT
#define OMIT_OBJECT_SET
#include "common.glsl"
#include "effects.glsl"

layout(location = 0) out vec4 out_colour;

layout(set = 2, binding = 0) uniform sampler2D tex;
layout(set = 2, binding = 1) uniform sampler3D lut;

layout(set = 2, binding = 2) uniform Params
{
    float gamma;
    float exposure;
    float offset;
    float use_lut;
};

void main()
{
    vec3 colour = texture(tex, frag.uv).rgb;
    if (use_lut > 0.5f)
        colour = sampleLut(colour, lut);
    out_colour = vec4(gammaAdjust(colour, gamma, exposure, offset), 1);
}