#pragma CANVAS_TRANSFORM
#pragma CANVAS_ATTACHMENTS

#include "common.glsl"
#include "pbr_util.glsl"

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
}

uniform sampler2D colour_tex;
uniform sampler2D normal_tex;
uniform sampler2D param_tex;
uniform sampler2D custom_tex;
uniform sampler2D depth_tex;

bool fragment(in Varyings vars, inout Fragment frag)
{
    vec4 colour_val = texture(colour_tex, vars.uv.xy);
    vec4 param_val = texture(param_tex, vars.uv.xy);
    if (param_val.w < 0.5f)
    {
        frag.colour = colour_val;
        return true;
    }
    vec4 normal_val = texture(normal_tex, vars.uv.xy);
    vec4 specular_val = texture(custom_tex, vars.uv.xy);
    float z = texture(depth_tex, vars.uv.xy).r;
    vec4 clip_position = vec4(vars.position.xy, z, 1.0f);
    vec4 view_position = scene.clip_to_view * clip_position;
    view_position /= view_position.w;
    vec4 world_position = scene.view_to_world * view_position;
    // TODO: view space lighting?

    frag.colour = vec4(pbrSurface(colour_val.rgb, world_position.xyz, normal_val.xyz, specular_val.rgb, param_val.r, param_val.g, param_val.b, scene.ambient_light.rgb, scene.eye_position), 1.0f);
    return true;
}