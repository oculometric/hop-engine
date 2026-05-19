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

bool fragment(in Varyings vars, inout Fragment frag)
{
    PBR_SETUP;

    frag.colour = vec4(pbrSurface(albedo_val.rgb, vars.position.xyz, perturbed_normal, specular_colour.rgb, pbr_val.r, pbr_val.g, pbr_val.b, scene.ambient_light.rgb, scene.eye_position), 1.0f);
    frag.normal = vec4(perturbed_normal, 1);
    frag.custom.w = 1;
    return true;
}