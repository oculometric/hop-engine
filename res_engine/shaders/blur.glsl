#pragma CANVAS_TRANSFORM
#pragma CANVAS_ATTACHMENTS

#include "res://engine/shaders/common.glsl"
#include "res://engine/shaders/effects.glsl"

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
}

uniform sampler2D main_tex;

bool fragment(in Varyings vars, out Fragment frag)
{
    evaluateKernel(gaussian_kernel_9, 9, main_tex, frag.colour);
    return true;
}
