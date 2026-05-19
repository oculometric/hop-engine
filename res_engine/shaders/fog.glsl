#pragma CANVAS_TRANSFORM
#pragma CANVAS_ATTACHMENTS

#include "res://engine/shaders/common.glsl"

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
}

uniform sampler2D screen_texture;
uniform sampler2D depth_texture;

uniform MaterialUniforms
{
    vec4 fog_colour;
    float fog_start;
    float fog_end;
    float fog_exponent;
};

bool fragment(in Varyings vars, inout Fragment frag)
{
    vec3 scene_colour = texture(screen_texture, vars.uv.xy).rgb;
    if (fog_start == fog_end || fog_exponent == 0)
    {
        frag.colour = vec4(scene_colour, 1);
        return true;
    }

    float d = texture(depth_texture, vars.uv).r;
    if (d == 1)
    {
        frag.colour = vec4(scene_colour, 1);
        return true;
    }
    float depth = scene.near_far.x * scene.near_far.y / (scene.near_far.y + d * (scene.near_far.x - scene.near_far.y));
    float fog_mix = pow((depth - fog_start) / (fog_end - fog_start), fog_exponent);
    frag.colour = vec4(mix(scene_colour, fog_colour.rgb, clamp(fog_mix, 0, 1)), 1);
    return true;
}