#version 450

#define FRAGMENT
#define NONSTANDARD_FRAG_OUT
#define OMIT_OBJECT_SET
#include "common.glsl"

layout(location = 0) out vec4 out_colour;

layout(set = 2, binding = 0) uniform sampler2D normal_texture;
layout(set = 2, binding = 1) uniform sampler2D depth_texture;

#include "dither.glsl"

#define NUM_SAMPLES 24

layout(set = 2, binding = 2) uniform AOParams
{
    vec4 samples[NUM_SAMPLES];
};

const float radius = 1.0f;
const float power = 2.0f;
const float bias = 0.025f;

const vec2 fbm_e = vec2(12.9898f,78.2330f);

float fbm_random(vec2 coord)
{
    return fract(sin(dot(coord, fbm_e)) * 43758.5f);
}

float clip_to_view_depth(vec4 clip_postion)
{
    mat4 cv = scene.clip_to_view;
    vec4 row_2 = vec4(cv[0].z, cv[1].z, cv[2].z, cv[3].z);
    vec4 row_3 = vec4(cv[0].w, cv[1].w, cv[2].w, cv[3].w);
    float z = dot(row_2, clip_postion);
    float w = dot(row_3, clip_postion);
    
    return z / w;
}

void main()
{
    // original normal of the pixel in world and view space
    vec3 world_normal = texture(normal_texture, frag.uv).xyz;
    vec3 view_normal = (scene.world_to_view * vec4(world_normal, 0.0f)).xyz;
    
    // compute view position and depth at current pixel - would it be better to store this to speed up access?
    float z = texture(depth_texture, frag.uv).r;
    vec4 clip_position = vec4(frag.position.xy, z, 1.0f);
    vec4 view_position = scene.clip_to_view * clip_position;
    view_position /= view_position.w;
    
    // use the dither matrix to compute randomized normal, tangent, bitangent vectors
    ivec2 pixel_uv = ivec2(floor(frag.uv * scene.viewport_size));
    float dither = (dither_map_4[(pixel_uv.x % 4) + ((pixel_uv.y % 4) * 4)] / 16.0f) * 2.0f - 1.0f;
    vec3 view_perp = cross(view_normal, vec3(0, 0, 1));
    vec3 view_perp2 = cross(view_perp, view_normal);
    
    vec3 view_tangent = normalize((sin(dither * 6.28f) * view_perp) + (cos(dither * 6.28f) * view_perp2));
    vec3 view_bitangent = cross(view_tangent, view_normal);
    view_tangent = normalize(cross(view_bitangent, view_normal));
    mat3x3 tbn = mat3x3(view_tangent, view_bitangent, view_normal);
    
    float occlusion = 0.0f;
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        vec3 sample_position = view_position.xyz + ((tbn * samples[i].xyz) * radius);
        vec4 sample_clip_position = scene.view_to_clip * vec4(sample_position, 1.0f);
        sample_clip_position /= sample_clip_position.w;
        
//        if (abs(sample_clip_position.x) > 1 || abs(sample_clip_position.y) > 1)
//            continue;
        
        float resample_z = texture(depth_texture, (sample_clip_position.xy * 0.5f) + 0.5f).r;
        vec4 resample_clip_position = vec4(sample_clip_position.xy, resample_z, 1.0f);
        float resample_view_z = clip_to_view_depth(resample_clip_position);
        
        if (resample_view_z >= sample_position.z + bias)
            occlusion += smoothstep(0.0f, 1.0f, radius / abs(resample_view_z - view_position.z));
    }
    
    occlusion = pow(smoothstep(0.0f, 1.0f, 1.0f - (occlusion / NUM_SAMPLES)), power);
    out_colour = vec4(vec3(occlusion), 1);
}