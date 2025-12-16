#version 450

#define VERTEX
#include "engine/common.glsl"

vec4 snap(vec4 value)
{
    vec2 snapping_value = vec2(scene.viewport_size) * 0.25f;
    vec2 rounding = value.xy / value.w;
    vec2 snapped = round(rounding * snapping_value) / snapping_value;
    snapped *= value.w;
    return vec4(snapped, value.z, value.w);
}

void main()
{
    frag.position = object.model_to_world * vec4(in_position.xyz, 1);
    frag.colour = in_colour;
    frag.normal = vec4(normalize((object.model_to_world * vec4(in_normal.xyz, 0)).xyz), 0);
    frag.tangent = vec4(normalize((object.model_to_world * vec4(in_tangent.xyz, 0)).xyz), 0);
    frag.uv = in_uv;

    gl_Position = snap(scene.view_to_clip * scene.world_to_view * frag.position);
}