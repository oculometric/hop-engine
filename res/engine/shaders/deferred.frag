#version 450

#define FRAGMENT
#include "common.glsl"
#include "pbr_util.glsl"

layout(set = 2, binding = 0) uniform sampler2D albedo_tex;
layout(set = 2, binding = 1) uniform sampler2D normal_tex;
layout(set = 2, binding = 2) uniform sampler2D pbr_tex;

layout(set = 2, binding = 3) uniform Params
{
    PBR_PARAMS;
};

void main()
{
    PBR_SETUP;
    
    // output format:
    // out_colour.rgb <- albedo
    // out_colour.a   <- EMPTY
    // out_normal.xyz <- normal with mapping applied
    // out_normal.w   <- EMPTY
    // out_params.x   <- roughness
    // out_params.y   <- metallic
    // out_params.z   <- emission strength
    // out_params.w   <- 1 if shading enabled, 0 if not
    // out_custom.rgb <- specular colour
    // out_custom.a   <- EMPTY
    out_colour = vec4(albedo_val.rgb, 1.0f);
    out_normal = vec4(perturbed_normal, 1.0f);
    out_params = vec4(pbr_val.xyz, 1.0f);
    out_custom = vec4(specular_colour.rgb, 1.0f);
}