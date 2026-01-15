#version 450

#define FRAGMENT
#include "common.glsl"

layout(set = 2, binding = 0) uniform sampler2D albedo_tex;
layout(set = 2, binding = 1) uniform sampler2D normal_tex;
layout(set = 2, binding = 2) uniform sampler2D pbr_tex;

layout(set = 2, binding = 3) uniform Params
{
    vec4 base_colour;           // base colour of the material. multiplied with all texture samples
    vec4 specular_colour;       // specular colour of the material

    float roughness_factor;     // surface roughness factor. blends between specular and diffuse lighting. multiplied with value in pbr texture
    float roughness_factor_add; // offset value for surface roughness factor
    float metallic_factor;      // surface metallic factor. blends between using specular colour and albedo colour for specular highlight. multiplied with value in pbr texture
    float metallic_factor_add;  // offset value for surface metallic factor
    float normal_strength;      // strength of the normal mapping effect
    float emission_strength;    // strength of emission

    float use_triplanar;        // set to >0 if you want to use triplanar mapping instead of UVs
    float triplanar_scale;      // scale of the texture when using triplanar mapping
};

void main()
{
    vec2 uv = frag.uv;
    if (use_triplanar > 0)
    {
        uv = abs(frag.normal.z) > 0.701f ? frag.position.xy : (abs(frag.normal.x) > 0.701f ? frag.position.yz : frag.position.xz);
        uv *= triplanar_scale;
    }
    
    vec4 albedo_val = texture(albedo_tex, uv);
    if (albedo_val.a < 0.5f) // TODO: dithered intead of scissoring
        discard;
    albedo_val.rgb *= base_colour.rgb;

    vec3 bitangent = normalize(cross(frag.tangent.xyz, frag.normal.xyz));
    mat3 tbn = mat3(frag.tangent.xyz, bitangent, frag.normal.xyz);
    vec3 normal_val = normalize((toSRGB(texture(normal_tex, uv).rgb) * 2.0f - 1.0f));
    normal_val.y *= -1;
    vec3 perturbed_normal = tbn * normal_val;
    
    vec4 pbr_val = texture(pbr_tex, uv);
    pbr_val *= vec4(roughness_factor, metallic_factor, emission_strength, 1.0f);
    pbr_val += vec4(roughness_factor_add, metallic_factor_add, 0.0f, 0.0f);
    
    // output format:
    // out_colour.rgb <- albedo
    // out_colour.a   <- EMPTY
    // out_normal.xyz <- normal with mapping applied
    // out_normal.w   <- EMPTY
    // out_params.x   <- roughness
    // out_params.y   <- metallic
    // out_params.z   <- emission strength
    // out_params.w   <- EMPTY
    // out_custom.rgb <- specular colour
    // out_custom.a   <- EMPTY
    out_colour = vec4(albedo_val.rgb * frag.colour.rgb, 1.0f);
    out_normal = vec4(normalize(mix(frag.normal.xyz, perturbed_normal, normal_strength)), 0.0f);
    out_params = vec4(pbr_val.xyz, 0.0f);
    out_custom = vec4(specular_colour.rgb, 0.0f);
}