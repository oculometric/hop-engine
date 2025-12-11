#version 450

#define FRAGMENT
#include "common.glsl"

layout(set = 2, binding = 0) uniform MaterialInfo
{
    Material material;
    Light light;
    vec4 ambient_colour;
};

layout(set = 2, binding = 1) uniform sampler2D albedo;
layout(set = 2, binding = 2) uniform sampler2D normal_map;


vec3 to_linear(vec3 srgb)
{
    return (pow(srgb, vec3(1.0f / 2.4f)) * 1.055f) -0.055f;
    //pow((srgb + 0.055f) / 1.055f, vec3(2.4f));
}

float saturate(float f) { return clamp(f, 0, 1); }

void main()
{
    vec3 pixel_to_light = light.position.xyz - frag.position.xyz;
    //vec3 pixel_to_eye = normalize(scene.eye_position - frag.position.xyz);
    //float light_distance = length(pixel_to_light);
    pixel_to_light = normalize(pixel_to_light);

    vec4 albedo_val = texture(albedo, frag.uv);
    vec3 normal_val = normalize((to_linear(texture(normal_map, frag.uv).rgb) * 2.0f - 1.0f));
    vec3 bitangent = normalize(cross(frag.tangent.xyz, frag.normal.xyz));
    mat3 tbn = mat3(frag.tangent.xyz, bitangent, frag.normal.xyz);
    vec3 perturbed_normal = normalize(tbn * normal_val.xyz);

    vec3 col = albedo_val.rgb * saturate(dot(pixel_to_light, perturbed_normal.xyz));
    col += ambient_colour.rgb;

    //if (light.light_type == 0)
    //{
    //    float attenuation = 1.0f / (light.constant_attenuation
    //                              + (light.linear_attenuation * light_distance) 
    //                              + (light.quadratic_attenuation * light_distance * light_distance));
    //    float n_dot_l = max(0, dot(frag.normal.xyz, pixel_to_light));
    //    vec3 diffuse_contribution = light.colour.rgb * n_dot_l * attenuation;
    //    float light_intensity = saturate(dot(frag.normal.xyz, pixel_to_light));
    //    float specular = 0.0f;
    //    if (light_intensity > 0.0f)
    //    {
    //        vec3 reflection = reflect(-pixel_to_light, frag.normal.xyz);
    //        float d = dot(reflection, pixel_to_eye);
    //        specular = pow(saturate(d * light_distance), material.specular_exponent);
    //    }
    //    float specular_contribution = specular * attenuation;
    //    
    //    col += material.emissive.rgb * albedo.rgb;
    //    col += material.specular.rgb * specular_contribution * albedo.rgb;
    //    col += material.diffuse.rgb * diffuse_contribution * albedo.rgb;
    //    col += material.ambient.rgb * ambient_colour.rgb * albedo.rgb;
    //}
    normal = vec4(perturbed_normal.xyz, 1);
    colour = vec4(col, 1);
}