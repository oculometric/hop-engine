#version 450

#define FRAGMENT
#include "common.glsl"

layout(set = 2, binding = 0) uniform MaterialInfo
{
    vec4 diffuse;
    vec4 specular;
    vec4 emissive;
    float specular_exponent;
    float specular_factor;
    float use_triplanar;
    float triplanar_scale;
} material;

layout(set = 2, binding = 1) uniform sampler2D albedo_tex;
layout(set = 2, binding = 2) uniform sampler2D normal_map;

float saturate(float f) { return clamp(f, 0, 1); }

vec3 sampleLight(Light light, vec3 alb, vec3 norm, vec3 pixel_to_eye)
{
    if (!light.enabled)
        return vec3(0);

    vec3 pixel_to_light = vec3(0);
    float distance_to_light = 1.0f;
    float attenuation = 1.0f;

    if (light.type == 0) // point light
    {
        pixel_to_light = light.position.xyz - frag.position.xyz;
        distance_to_light = length(pixel_to_light);
        pixel_to_light = normalize(pixel_to_light);
        attenuation = 1.0f / (distance_to_light * distance_to_light);
    }
    else if (light.type == 1)
    {
        pixel_to_light = light.position.xyz - frag.position.xyz;
        distance_to_light = length(pixel_to_light);
        pixel_to_light = normalize(pixel_to_light);
        attenuation = 1.0f / (distance_to_light * distance_to_light);

        float l_dot_d = saturate(-dot(pixel_to_light, light.direction.xyz));
        if (acos(l_dot_d) > light.spot_angle * (3.1415f / 180.0f))
            attenuation = 0;
    }
    else if (light.type == 2) // directional
    {
        pixel_to_light = -light.direction.xyz;
    }
    else
        return vec3(0);

    float n_dot_l = 0;
    if (dot(pixel_to_light, frag.normal.xyz) > 0.0f) // TODO: backside lighting
        n_dot_l = saturate(dot(pixel_to_light, norm));
    float specular = 0.0f;
    if (n_dot_l > 0.0f)
    {
        vec3 reflection = reflect(-pixel_to_light, norm);
        float d = dot(reflection, pixel_to_eye);
        specular = pow(saturate(d / distance_to_light), material.specular_exponent);
    }

    vec3 result = material.emissive.rgb
                + (alb * material.diffuse.rgb * light.colour.rgb * frag.colour.rgb * n_dot_l * attenuation)
                + (material.specular.rgb * light.colour.rgb * specular * attenuation * material.specular_factor);

    return result;
}

void main()
{
    vec3 pixel_to_eye = normalize(scene.eye_position - frag.position.xyz);

    vec2 uv = frag.uv;
    if (material.use_triplanar > 0)
    {
        uv = abs(frag.normal.z) > 0.701f ? frag.position.xy : (abs(frag.normal.x) > 0.701f ? frag.position.yz : frag.position.xz);
        uv *= material.triplanar_scale;
    }
    
    vec4 albedo = texture(albedo_tex, uv);
    if (albedo.a < 0.5f)
        discard;
    vec3 normal_val = normalize((toSRGB(texture(normal_map, uv).rgb) * 2.0f - 1.0f));
    normal_val.y *= -1;
    vec3 bitangent = normalize(cross(frag.tangent.xyz, frag.normal.xyz));
    mat3 tbn = mat3(frag.tangent.xyz, bitangent, frag.normal.xyz);
    vec3 perturbed_normal = normalize(tbn * normal_val.xyz);

    vec3 col = scene.ambient_light.rgb * albedo.rgb * frag.colour.rgb;
    for (uint i = 0; i < 8; ++i)
    {
        col += sampleLight(scene.lights[i], albedo.rgb, perturbed_normal, pixel_to_eye);
    }

    out_colour = vec4(col, 1);
    out_normal = vec4(perturbed_normal, 1);
}