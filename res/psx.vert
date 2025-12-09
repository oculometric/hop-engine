#version 450

#define VERTEX
#include "common.glsl"

vec4 snap(vec4 value)
{
    vec4 snapping_value = vec4(vec2(scene.viewport_size) * 0.125f, 1024, 1);
    vec4 snapped = round(value * snapping_value) / snapping_value;
    snapped.w = value.w;
    return snapped;
}

void main()
{
    frag.position = (object.model_to_world * vec4(position.xyz, 1));
    frag.colour = colour;
    frag.normal = vec4(normalize((object.model_to_world * vec4(normal.xyz, 0)).xyz), 0);
    frag.tangent = vec4(normalize((object.model_to_world * vec4(tangent.xyz, 0)).xyz), 0);
    frag.uv = uv;

    gl_Position = snap(scene.view_to_clip * scene.world_to_view * object.model_to_world * vec4(position.xyz, 1.0));
}