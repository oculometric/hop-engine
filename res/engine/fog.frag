#version 450

#define FRAGMENT
#define NONSTANDARD_FRAG_OUT
#define OMIT_OBJECT_SET
#include "common.glsl"

layout(location = 0) out vec4 out_colour;

layout(set = 2, binding = 0) uniform sampler2D screen_texture;
layout(set = 2, binding = 1) uniform sampler2D depth_texture;

layout(set = 2, binding = 2) uniform MaterialUniforms
{
    vec4 fog_colour;
    float fog_start;
    float fog_end;
    float fog_exponent;
};

void main()
{
    vec3 scene_colour = texture(screen_texture, frag.uv).rgb;
    if (fog_start == fog_end || fog_exponent == 0)
    {
        out_colour = vec4(scene_colour, 1);
        return;
    }
    
    float d = texture(depth_texture, frag.uv).r;
    if (d == 1)
    {
        out_colour = vec4(scene_colour, 1);
        return;
    }
    float depth = scene.near_far.x * scene.near_far.y / (scene.near_far.y + d * (scene.near_far.x - scene.near_far.y));
    float fog_mix = pow((depth - fog_start) / (fog_end - fog_start), fog_exponent);
    out_colour = vec4(mix(scene_colour, fog_colour.rgb, clamp(fog_mix, 0, 1)), 1);
}