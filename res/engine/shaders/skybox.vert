#version 450

#define VERTEX
#include "common.glsl"

void main()
{
    frag.uv = in_uv;

    mat4 to_clip = scene.world_to_view;
    to_clip[0] = normalize(to_clip[0]);
    to_clip[1] = normalize(to_clip[1]);
    to_clip[2] = normalize(to_clip[2]);
    to_clip[3] = vec4(0, 0, 0, 1);
    gl_Position = scene.view_to_clip * to_clip * vec4(in_position.xyz, 1.0);
}