#version 450

#define FRAGMENT
#define NONSTANDARD_FRAG_OUT
#define OMIT_OBJECT_SET
#include "common.glsl"
#include "effects.glsl"

layout(location = 0) out vec4 out_colour;

layout(set = 2, binding = 0) uniform sampler2D normal_texture;
layout(set = 2, binding = 1) uniform sampler2D depth_texture;

layout(set = 2, binding = 2) uniform AOParams
{
    vec4 samples[NUM_SSAO_SAMPLES];
};

void main()
{
    out_colour = vec4(vec3(computeSSAO(1.0f, 2.0f, 0.025f, frag.uv, normal_texture, depth_texture, samples)), 1);
}