#version 450

#define FRAGMENT
#include "engine/shaders/common.glsl"

layout(set = 2, binding = 1) uniform sampler2D albedo;

const vec3 highlight = vec3(1.300f, 1.300f, 1.300f);
const vec3 shadow = vec3(0.194f, 0.129f, 0.076f);

void main()
{
    vec4 col = texture(albedo, frag.uv);
    if (col.a < 0.5f)
        discard;

    float n_dot_l = dot(-scene.lights[0].direction.xyz, frag.normal.xyz) * 3.0f;
    if (n_dot_l < 0.061f)
        out_colour = vec4(col.rgb * shadow, 1);
    else if (n_dot_l < 0.82f)
        out_colour = vec4(mix(col.rgb, col.rgb * shadow, 1.0f - 0.341f), 1);
    else if (n_dot_l > 2.20f)
        out_colour = vec4(mix(col.rgb, col.rgb * highlight, 0.76f), 1);
    else
        out_colour = vec4(col.rgb, 1);
    out_normal = vec4(frag.normal.xyz, 0);
    out_custom = vec4(frag.position.xyz, 0);
    out_params = vec4(frag.colour.xyz, 0);
}