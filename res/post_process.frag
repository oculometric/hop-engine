#version 450

#define FRAGMENT
#define NONSTANDARD_FRAG_OUT
#define OMIT_OBJECT_SET
#include "common.glsl"

layout(location = 0) out vec4 colour;

layout(set = 2, binding = 0) uniform sampler2D screen_texture;
layout(set = 2, binding = 1) uniform sampler2D normal_texture;
layout(set = 2, binding = 2) uniform sampler2D params_texture;
layout(set = 2, binding = 3) uniform sampler2D custom_texture;
layout(set = 2, binding = 4) uniform sampler2D depth_texture;

#include "dither.glsl"

#define NUM_SAMPLES 64

layout(set = 2, binding = 5) uniform AOParams
{
    vec4 samples[NUM_SAMPLES];
};

const float radius = 0.5f;

float calculateOcclusion()
{
    vec2 screen_uv = frag.uv;
    
    vec3 world_normal = texture(normal_texture, screen_uv).xyz;
    vec3 view_normal = normalize((scene.world_to_view * vec4(world_normal, 0.0f)).xyz);
    
    float z = texture(depth_texture, screen_uv).r;
    vec4 clip_position = vec4(frag.position.xy, z, 1.0f);
    vec4 view_position = scene.clip_to_view * clip_position;
    view_position /= view_position.w;
    float linear_depth = -view_position.z;
    
    ivec2 pixel_uv = ivec2(floor(screen_uv * scene.viewport_size));
    float dither = (dither_map_4[(pixel_uv.x % 4) + ((pixel_uv.y % 4) * 4)] / 16.0f) * 2.0f - 1.0f;
    
    vec3 view_normal_perp_1 = view_normal.zxy;
    vec3 view_normal_perp_2 = view_normal.yzx;
    float anti_dither = sqrt(1.0f - (dither * dither));

    vec3 view_tangent = (dither * view_normal_perp_1) + (anti_dither * view_normal_perp_2);
    vec3 view_bitangent = normalize(cross(view_tangent, view_normal));
    view_tangent = normalize(cross(view_bitangent, view_normal));
    mat3x3 tbn = mat3x3(view_tangent, view_bitangent, view_normal);
    
    float occlusion = 0.0f;
    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        vec3 view_sample = view_position.xyz + ((tbn * samples[i].xyz) * radius);
        vec4 sample_ndc = scene.view_to_clip * vec4(view_sample, 1.0f);
        sample_ndc /= sample_ndc.w;
        float linear_sample_depth = -view_sample.z;
        
        if (abs(sample_ndc.x) > 1 || abs(sample_ndc.y) > 1)
            continue;
        
        float sample_z = texture(depth_texture, (sample_ndc.xy * 0.5f) + 0.5f).x;
        vec4 resample_ndc = vec4(view_sample.xy, sample_z, 1.0f);
        vec4 view_resample = scene.clip_to_view * resample_ndc;
        view_resample /= view_resample.w;
        float linear_resample_depth = -view_resample.z;
        
        float r = smoothstep(0.0f, 1.0f, radius / abs(linear_depth - linear_resample_depth));
        occlusion += ((linear_sample_depth > linear_resample_depth + 0.025f) ? 1.0f : 0.0f) * r;
    }
    
    occlusion = pow(smoothstep(0.0f, 1.0f, 1.0f - (occlusion / NUM_SAMPLES)), 3.0f);
    
    return occlusion;
}


void main()
{
    colour = vec4(texture(screen_texture, frag.uv).rgb, 1);
    //vec3 col = texture(screen_texture, frag.uv).rgb;
    //colour = vec4(col * calculateOcclusion(), 1);
}