#version 450

#define FRAGMENT
#include "engine/common.glsl"

layout(set = 2, binding = 1) uniform sampler2D albedo;

void main()
{
    vec4 col = texture(albedo, frag.uv);
    if (col.a < 0.5f)
        discard;
    out_colour = vec4(col.rgb, 1);
    out_normal = vec4(frag.normal.xyz, 0);
    out_custom = vec4(frag.position.xyz, 0);
}