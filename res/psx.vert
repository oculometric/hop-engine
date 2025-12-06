#version 450

#define VERTEX
#include "common.glsl"

vec4 snap(vec4 value)
{
    vec4 snapping_value = vec4(128, 128, 1024, 1);
    vec4 snapped = round(value * snapping_value) / snapping_value;
    snapped.w = value.w;
    return snapped;
}

void main()
{
    frag.position = (object.model_to_world * vec4(position, 1)).xyz;
    frag.colour = colour;
    frag.normal = normalize((object.model_to_world * vec4(normal, 0)).xyz);
    frag.tangent = normalize((object.model_to_world * vec4(tangent, 0)).xyz);
    frag.uv = uv;

    gl_Position = snap(scene.view_to_clip * scene.world_to_view * object.model_to_world * vec4(position.xyz, 1.0));
}