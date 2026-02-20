#pragma DEFAULT_VERTEX

void vertex()
{
    #pragma DEFAULT_TRANSFORM
}

#pragma DEFAULT_ATTACHMENTS

#include "common.glsl"
#include "pbr_util.glsl"

layout(set = 2, binding = 0) uniform sampler2D albedo_tex;
layout(set = 2, binding = 1) uniform sampler2D normal_tex;
layout(set = 2, binding = 2) uniform sampler2D pbr_tex;

layout(set = 2, binding = 3) uniform Params
{
    PBR_PARAMS;
};

void fragment()
{
    PBR_SETUP;

    out_colour = vec4(pbrSurface(albedo_val.rgb, frag.position.xyz, perturbed_normal, specular_colour.rgb, pbr_val.r, pbr_val.g, pbr_val.b, scene.ambient_light.rgb, scene.eye_position), 1.0f);

    out_normal = vec4(perturbed_normal, 1);
    out_params.w = 0.0f;
}