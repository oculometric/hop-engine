#version 450

#define FRAGMENT
#define NONSTANDARD_FRAG_OUT
#define OMIT_OBJECT_SET
#include "common.glsl"

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

vec3 gammaAdjust(vec3 value)
{
    return (exposure * pow(value + offset, vec3(gamma)));
}

vec3 sampleLut(vec3 colour)
{
    vec3 linear = toSRGB((colour * (63.0f / 64.0f)) + (0.5f / 64.0f));
    vec3 flipped = vec3(linear.r, linear.g, linear.b);
    return (textureLod(lut, flipped, 0.0f).rgb);
}

void main()
{
    if (use_lut > 0.5f)
        out_colour = vec4(gammaAdjust(sampleLut(texture(tex, frag.uv).rgb)), 1);
    else
        out_colour = vec4(gammaAdjust(texture(tex, frag.uv).rgb), 1);
}