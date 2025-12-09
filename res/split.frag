#version 450

#define FRAGMENT
#include "common.glsl"

layout(set = 2, binding = 0) uniform MaterialInfo
{
    Material material;
    Light light;
    vec4 ambient_colour;
};

layout(set = 2, binding = 1) uniform sampler2D tex;
layout(set = 2, binding = 2) uniform sampler2D tex2;

float saturate(float f) { return clamp(f, 0, 1); }

void main()
{
    vec3 pixel_to_light = light.position.xyz - frag.position.xyz;
    vec3 pixel_to_eye = normalize(scene.eye_position - frag.position.xyz);
    float light_distance = length(pixel_to_light);
    pixel_to_light = normalize(pixel_to_light);

    vec4 albedo = mix(texture(tex, frag.uv), texture(tex2, frag.uv), float(frag.position.x < 0.0f));
    vec3 col = vec3(0);

    if (light.light_type == 0)
    {
        float attenuation = 1.0f / (light.constant_attenuation
                                  + (light.linear_attenuation * light_distance) 
                                  + (light.quadratic_attenuation * light_distance * light_distance));
        float n_dot_l = max(0, dot(frag.normal.xyz, pixel_to_light));
        vec3 diffuse_contribution = light.colour.rgb * n_dot_l * attenuation;
        float light_intensity = saturate(dot(frag.normal.xyz, pixel_to_light));
        float specular = 0.0f;
        if (light_intensity > 0.0f)
        {
            vec3 reflection = reflect(-pixel_to_light, frag.normal.xyz);
            float d = dot(reflection, pixel_to_eye);
            specular = pow(saturate(d * light_distance), material.specular_exponent);
        }
        float specular_contribution = specular * attenuation;
        
        col += material.emissive.rgb * albedo.rgb;
        col += material.specular.rgb * specular_contribution * albedo.rgb;
        col += material.diffuse.rgb * diffuse_contribution * albedo.rgb;
        col += material.ambient.rgb * ambient_colour.rgb * albedo.rgb;
    }
    colour = vec4(col, albedo.a);
}