#version 450

#define FRAGMENT
#define NONSTANDARD_FRAG_OUT
#define OMIT_OBJECT_SET
#include "common.glsl"

layout(location = 0) out vec4 colour;

layout(set = 2, binding = 0) uniform sampler2D screen_texture;
layout(set = 2, binding = 1) uniform sampler2D normal_texture;
layout(set = 2, binding = 2) uniform sampler2D params_texture;
layout(set = 2, binding = 3) uniform sampler2D custom_texture;
layout(set = 2, binding = 4) uniform sampler2D depth_texture;

void main()
{
    float depth = texture(depth_texture, frag.uv).r;
    float near = scene.near_far.x;
    float far = scene.near_far.y;
    depth = near * far / (far + depth * (near - far));
    colour = vec4(vec3(depth), 1);
}