#version 450

#define FRAGMENT
#include "common.glsl"

layout(set = 2, binding = 1) uniform sampler2D albedo;

void main()
{
    vec4 col = texture(albedo, frag.uv);
    if (col.a < 0.5f)
        discard;
    colour = vec4(col.rgb, 1);
    normal = vec4(frag.normal.xyz, 0);
    custom = vec4(frag.position.xyz, 0);
}