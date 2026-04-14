#pragma CANVAS_TRANSFORM

#include "common.glsl"
#include "pbr_util.glsl"

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
}

uniform sampler2D albedo_tex;
uniform sampler2D normal_tex;
uniform sampler2D pbr_tex;

uniform Params
{
    PBR_PARAMS;
};

bool fragment(in Varyings vars, out Fragment frag)
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
    frag.colour = vec4(albedo_val.rgb, 1.0f);
    frag.normal = vec4(perturbed_normal, 1.0f);
    frag.params = vec4(pbr_val.xyz, 1.0f);
    frag.custom = vec4(specular_colour.rgb, 1.0f);
    return true;
}