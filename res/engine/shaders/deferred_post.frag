#version 450

#define FRAGMENT
#define NONSTANDARD_FRAG_OUT
#define OMIT_OBJECT_SET
#include "common.glsl"

layout(location = 0) out vec4 out_colour;

layout(set = 2, binding = 0) uniform sampler2D colour_tex;
layout(set = 2, binding = 1) uniform sampler2D normal_tex;
layout(set = 2, binding = 2) uniform sampler2D param_tex;
layout(set = 2, binding = 3) uniform sampler2D custom_tex;
layout(set = 2, binding = 4) uniform sampler2D depth_tex;

float saturate(float f) { return min(max(f, 0.0f), 1.0f); }

vec3 pbrSurface(vec3 albedo, vec3 position, vec3 mapped_normal, vec3 specular_colour, float roughness, float metallic, float emission)
{
    vec3 pixel_to_eye = normalize(scene.eye_position - position);
    
    vec3 final_colour = mix(vec3(0.0f), scene.ambient_light.rgb * albedo, roughness);
    for (uint i = 0; i < 8; ++i)
    {
        Light light = scene.lights[i];
        
        if (!light.enabled)
            continue;

        // TODO: shadow mapping

        vec3 light_dir;
        float light_strength = light.colour.w;
        if (light.type == 0 || light.type == 1) // point or spot light
        {
            vec3 light_vec = position - light.position.xyz;
            light_dir = normalize(light_vec);
            light_strength /= (length(light_vec) * length(light_vec)) + 1;

            if (light.type == 1)
                if (light.spot_angle < 180)
                    light_strength *= pow(saturate((light.spot_angle - degrees(acos(dot(light_dir, light.direction.xyz)))) / light.spot_angle), 0.5);
        }
        else
        {
            light_dir = normalize(light.direction.xyz);
        }

        // light colour with strength applied
        vec3 light_col = light.colour.rgb * light_strength;
        // light direction
        vec3 w_i = -light_dir;
        // view direction
        vec3 w_o = -pixel_to_eye;
        // surface normal
        vec3 n = mapped_normal;
        // roughness factor
        float a = roughness;
        // dot product between the light direction and the surface normal
        float wi_dot_n = dot(w_i, n);
        // dot product between the view direction and the surface normal
        float wo_dot_n = dot(w_o, n);
        // vector which is halfway between the light vector and surface normal
        vec3 h = normalize(w_i + n);
        // Trowbridge-Reitz GGX normal distribution function, ref https://learnopengl.com/PBR/Theory
        float n_dot_h = dot(n, h);
        float a2 = a * a;
        float d = (max(n_dot_h, 0.0f) * max(n_dot_h, 0.0f) * (a2 - 1.0f)) + 1.0f;
        float ndf_trggx = a2 / (3.14159f * d * d);
        // Schlick GGX geometry function, ref as above
        float k = ((a + 1) * (a + 1)) / 8;
        float gf_sggx = (saturate(wo_dot_n) / ((saturate(wo_dot_n) * (1.0f - k)) + k))
            * (saturate(wi_dot_n) / ((saturate(wi_dot_n) * (1.0f - k)) + k)); // modified to add saturate functions
        // Fresnel-Schlick fresnel approximation, ref as above
        vec3 f0 = mix(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
        vec3 ff_fs = f0 + ((1.0f - f0) * pow(saturate(1.0f - max(n_dot_h, 0.0f)), 5.0f));
        // Cook-Torrance specular BRDF, ref as above
        vec3 f_ct = (ndf_trggx * gf_sggx * ff_fs) / ((4.0f * max(wi_dot_n, 0.0f) * max(wo_dot_n, 0.0f)) + 0.0001f);
        // Lambertian diffuse BRDF, ref as above
        vec3 f_lambert = albedo / 3.14159f;
        vec3 k_s = ff_fs;
        vec3 k_d = float3(1.0f, 1.0f, 1.0f) - k_s;
        k_d *= 1.0f - metallic;
        // Cook-Torrance BRDF (combined), ref as above
        vec3 f_r = ((k_d * f_lambert) + (f_ct));
        vec3 l = f_r * saturate(wi_dot_n) * light_col;

        final_colour += l;
    }
    final_colour += albedo * emission;
    
    return final_colour;   
}


void main()
{
    vec4 colour_val = texture(colour_tex, frag.uv);
    vec4 param_val = texture(param_tex, frag.uv);
    float z = texture(depth_tex, frag.uv).r;
    vec4 clip_position = vec4(frag.position.xy, z, 1.0f);
    vec4 view_position = scene.clip_to_view * clip_position;
    view_position /= view_position.w;
    vec4 world_position = inverse(scene.world_to_view) * view_position;
    // TODO: view space lighting?
    
    out_colour = pbrSurface(colour_val.rgb, world_position, texture(normal_tex, frag.uv).rgb, texture(custom_tex, frag.uv).rgb, param_val.r, param_val.g, colour_val.a);
}