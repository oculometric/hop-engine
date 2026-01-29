#version 450

#define FRAGMENT
#define NONSTANDARD_FRAG_OUT
#define OMIT_OBJECT_SET
#include "common.glsl"
#include "effects.glsl"

layout(location = 0) out vec4 out_colour;

layout(set = 2, binding = 0) uniform sampler2D main_tex;

void main()
{
    evaluateKernel(gaussian_kernel_9, 9, main_tex, out_colour);
}