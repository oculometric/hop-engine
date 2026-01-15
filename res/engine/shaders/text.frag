#version 450

#define FRAGMENT
#include "common.glsl"

layout(set = 2, binding = 0) uniform sampler2D text_atlas;

void main()
{
    vec2 uv = frag.uv;
    if (texture(text_atlas, uv).r < 0.5f)
        discard;
    
    out_colour = vec4(frag.colour.rgb, 1);
    out_params.w = 0.0f;
}