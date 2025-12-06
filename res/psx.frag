#version 450

#define FRAGMENT
#include "common.glsl"

layout(set = 2, binding = 1) uniform sampler2D albedo;

void main()
{
    vec4 col = texture(albedo, frag.uv * vec2(1, -1)).bgra;
    if (col.a < 0.5f)
        discard;
    colour = vec4(col.rgb, 1);
}