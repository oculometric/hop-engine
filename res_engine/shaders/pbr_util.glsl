// base colour of the material. multiplied with all texture samples
// specular colour of the material
// surface roughness factor. blends between specular and diffuse lighting. multiplied with value in pbr texture
// offset value for surface roughness factor
// surface metallic factor. blends between using specular colour and albedo colour for specular highlight. multiplied with value in pbr texture
// offset value for surface metallic factor
// strength of the normal mapping effect
// strength of emission
// set to >0 if you want to use triplanar mapping instead of UVs
// scale of the texture when using triplanar mapping
#define PBR_PARAMS \
    vec4 base_colour; \
    vec4 specular_colour; \
    float roughness_factor; \
    float roughness_factor_add; \
    float metallic_factor; \
    float metallic_factor_add; \
    float normal_strength; \
    float emission_strength; \
    float use_triplanar; \
    float triplanar_scale

vec2 calculateTriplanar(vec3 position, vec3 normal)
{
    return abs(normal.z) > 0.701f ? position.xy : (abs(normal.x) > 0.701f ? position.yz : position.xz);
}

vec3 calculateMappedNormal(vec3 normal, vec3 tangent, vec4 normal_from_texture)
{
    vec3 bitangent = cross(tangent, normal);
    mat3 tbn = mat3(tangent, bitangent, normal);
    vec3 normal_val = (toSRGB(normal_from_texture.rgb) * 2.0f - 1.0f);
    normal_val.y *= -1;
    return tbn * normal_val;
}

#include "dither.glsl"

#define PBR_SETUP \
    vec2 uv = use_triplanar > 0 ? calculateTriplanar(vars.position.xyz, vars.normal.xyz) * triplanar_scale : vars.uv.xy; \
    vec4 albedo_val = texture(albedo_tex, uv); \
    if (dither_4x4(albedo_val.a, pixelCoord((gl_FragCoord.xy * 0.5f) + 0.5f, scene.viewport_size)) < 1) \
        return false; \
    albedo_val.rgb *= base_colour.rgb * vars.colour.rgb; \
    vec3 perturbed_normal = calculateMappedNormal(vars.normal.xyz, vars.tangent.xyz, texture(normal_tex, uv)); \
    perturbed_normal = normalize(mix(vars.normal.xyz, perturbed_normal, normal_strength)); \
    vec4 pbr_val = texture(pbr_tex, uv); \
    pbr_val *= vec4(roughness_factor, metallic_factor, emission_strength, 1.0f); \
    pbr_val += vec4(roughness_factor_add, metallic_factor_add, 0.0f, 0.0f)

vec3 pbrSurface(vec3 albedo, vec3 position, vec3 mapped_normal, vec3 specular_colour, float roughness, float metallic, float emission, vec3 ambient_light, vec3 eye_position)
{
    vec3 pixel_to_eye = normalize(eye_position - position);

    vec3 final_colour = mix(vec3(0.0f), ambient_light * albedo, roughness);
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
        vec3 w_o = pixel_to_eye;
        // surface normal
        vec3 n = mapped_normal;
        // roughness factor
        float a = roughness;
        // dot product between the light direction and the surface normal
        float wi_dot_n = saturate(dot(w_i, n));
        // dot product between the view direction and the surface normal
        float wo_dot_n = saturate(dot(w_o, n));
        // vector which is halfway between the light vector and surface normal
        vec3 h = normalize(w_i + n);
        // Trowbridge-Reitz GGX normal distribution function, ref https://learnopengl.com/PBR/Theory
        float n_dot_h = saturate(dot(n, h));
        float a2 = a * a;
        float d = (n_dot_h * n_dot_h * (a2 - 1.0f)) + 1.0f;
        float ndf_trggx = a2 / (3.14159f * d * d);
        // Schlick GGX geometry function, ref as above
        float k = ((a + 1) * (a + 1)) / 8;
        float gf_sggx = (wo_dot_n / ((wo_dot_n * (1.0f - k)) + k))
        * (wi_dot_n / ((wi_dot_n * (1.0f - k)) + k));
        // Fresnel-Schlick fresnel approximation, ref as above
        vec3 f0 = mix(vec3(0.04f, 0.04f, 0.04f), specular_colour, metallic);
        vec3 ff_fs = f0 + ((1.0f - f0) * pow(1.0f - n_dot_h, 5.0f));
        // Cook-Torrance specular BRDF, ref as above
        vec3 f_ct = (ndf_trggx * gf_sggx * ff_fs) / ((4.0f * wi_dot_n * wo_dot_n) + 0.0001f);
        // Lambertian diffuse BRDF, ref as above
        vec3 f_lambert = albedo / 3.14159f;
        vec3 k_s = ff_fs;
        vec3 k_d = vec3(1.0f, 1.0f, 1.0f) - k_s;
        k_d *= 1.0f - metallic;
        // Cook-Torrance BRDF (combined), ref as above
        vec3 f_r = ((k_d * f_lambert) + (f_ct));
        vec3 l = f_r * wi_dot_n * light_col;

        final_colour += l;
    }
    final_colour += albedo * emission;

    return final_colour;
}