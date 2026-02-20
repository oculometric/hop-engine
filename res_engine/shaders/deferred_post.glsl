#pragma DEFAULT_VERTEX

void vertex()
{
    #pragma CANVAS_TRANSFORM
}

#pragma CANVAS_ATTACHMENTS

#include "common.glsl"
#include "pbr_util.glsl"

layout(set = 2, binding = 0) uniform sampler2D colour_tex;
layout(set = 2, binding = 1) uniform sampler2D normal_tex;
layout(set = 2, binding = 2) uniform sampler2D param_tex;
layout(set = 2, binding = 3) uniform sampler2D custom_tex;
layout(set = 2, binding = 4) uniform sampler2D depth_tex;

void fragment()
{
    vec4 colour_val = texture(colour_tex, frag.uv);
    vec4 param_val = texture(param_tex, frag.uv);
    if (param_val.w < 0.5f)
    {
        out_colour = colour_val;
        return;
    }
    vec4 normal_val = texture(normal_tex, frag.uv);
    vec4 specular_val = texture(custom_tex, frag.uv);
    float z = texture(depth_tex, frag.uv).r;
    vec4 clip_position = vec4(frag.position.xy, z, 1.0f);
    vec4 view_position = scene.clip_to_view * clip_position;
    view_position /= view_position.w;
    vec4 world_position = scene.view_to_world * view_position;
    // TODO: view space lighting?

    out_colour = vec4(pbrSurface(colour_val.rgb, world_position.xyz, normal_val.xyz, specular_val.rgb, param_val.r, param_val.g, param_val.b, scene.ambient_light.rgb, scene.eye_position), 1.0f);
}