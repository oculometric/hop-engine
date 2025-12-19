#version 450

#define FRAGMENT
#define NONSTANDARD_FRAG_OUT
#define OMIT_OBJECT_SET
#include "common.glsl"

layout(location = 0) out vec4 out_colour;

layout(set = 2, binding = 0) uniform sampler2D screen_texture;

layout(set = 2, binding = 1) uniform MaterialUniforms
{
    int display_depth;
};

void main()
{
    if (display_depth == 1)
    {
        float d = texture(screen_texture, frag.uv).r;
        float depth = scene.near_far.x * scene.near_far.y / (scene.near_far.y + d * (scene.near_far.x - scene.near_far.y));
        out_colour = vec4(vec3(depth), 1);
    }
    else if (display_depth == 2)
    {
        float d = uint(texture(screen_texture, frag.uv).r) / 256.0f;
        out_colour = vec4(vec3(d), 1);
    }
    else
        out_colour = vec4(texture(screen_texture, frag.uv).rgb, 1);
}