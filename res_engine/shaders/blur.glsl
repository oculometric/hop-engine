#include "res://engine/common.glsl"
#include "res://effects.glsl"

// uniforms have 'layout ... (set = 2, binding = n) ...' inserter automatically
uniform sampler2D main_tex;

#pragma DEFAULT_VERTEX

void vertex()
{
    #pragma CANVAS_TRANSFORM
}

#pragma CANVAS_ATTACHMENTS

void fragment()
{
    evaluateKernel(gaussian_kernel_9, 9, main_tex, out_colour);
}
