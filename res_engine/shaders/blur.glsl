#include "res://engine/shaders/common.glsl"
#include "res://engine/shaders/effects.glsl"

layout(set = 2, binding = 0) uniform sampler2D main_tex;

void vertex()
{
    #pragma CANVAS_TRANSFORM
}

#pragma CANVAS_ATTACHMENTS

void fragment()
{
    evaluateKernel(gaussian_kernel_9, 9, main_tex, out_colour);
}
