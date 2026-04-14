#pragma CANVAS_TRANSFORM
#pragma CANVAS_ATTACHMENTS

#include "res://engine/shaders/common.glsl"
#include "res://engine/shaders/effects.glsl"

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
}

uniform sampler2D normal_texture;
uniform sampler2D depth_texture;

uniform AOParams
{
    vec4 samples[NUM_SSAO_SAMPLES];
};

bool fragment(in Varyings vars, out Fragment frag)
{
    frag.colour = vec4(vec3(computeSSAO(1.0f, 2.0f, 0.025f, vars.uv.xy, vars.position.xy, normal_texture, depth_texture, samples)), 1);
    return true;
}