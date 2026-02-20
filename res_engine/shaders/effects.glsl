// GENERIC CONVOLUTION KERNEL ===============================================
float[25] gaussian_kernel_5 = float[](
    1, 2, 4, 2, 1,
    2, 4, 8, 4, 2,
    4, 8, 16, 8, 4,
    2, 4, 8, 4, 2,
    1, 2, 4, 2, 1
);

float[81] gaussian_kernel_9 = float[](
     1,  2,  4,   8,  16,   8,  4,  2,  1,
     2,  4,  8,  16,  32,  16,  8,  4,  2,
     4,  8, 16,  32,  64,  32, 16,  8,  4,
     8, 16, 32,  64, 128,  64, 32, 16,  8,
    16, 32, 64, 128, 256, 128, 64, 32, 16,
     8, 16, 32,  64, 128,  64, 32, 16,  8,
     4,  8, 16,  32,  64,  32, 16,  8,  4,
     2,  4,  8,  16,  32,  16,  8,  4,  2,
     1,  2,  4,   8,  16,   8,  4,  2,  1
);

#define evaluateKernel(kern, kernel_size, tex, out_val) \
{ \
    vec2 off = 1.0f / textureSize(tex, 0); \
    float kernel_factor_inv = 0.0f; \
    for (int i = 0; i < kernel_size * kernel_size; ++i) \
        kernel_factor_inv += kern[i]; \
    kernel_factor_inv = 1.0f / kernel_factor_inv; \
     \
    vec4 colour = vec4(0, 0, 0, 0); \
    vec2 uv = frag.uv - (off * floor(kernel_size / 2)); \
    vec2 uv_start = uv; \
    int i = 0; \
    for (int x = 0; x < kernel_size; ++x) \
    { \
        for (int y = 0; y < kernel_size; ++y) \
        { \
            colour += kern[i] * texture(tex, uv); \
            uv.x += off.x; \
            ++i; \
        } \
        uv.y += off.y; \
        uv.x = uv_start.x; \
    } \
     \
    out_val = colour * kernel_factor_inv; \
} \

// SSAO =========================
#include "dither.glsl"

#define NUM_SSAO_SAMPLES 24

float clip_to_view_depth(vec4 clip_postion)
{
    mat4 cv = scene.clip_to_view;
    vec4 row_2 = vec4(cv[0].z, cv[1].z, cv[2].z, cv[3].z);
    vec4 row_3 = vec4(cv[0].w, cv[1].w, cv[2].w, cv[3].w);
    float z = dot(row_2, clip_postion);
    float w = dot(row_3, clip_postion);

    return z / w;
}

// this is based closely on the LearnOpenGL example - https://learnopengl.com/Advanced-Lighting/SSAO
float computeSSAO(float radius, float power, float bias, vec2 uv, vec2 frag_position, sampler2D normal_texture, sampler2D depth_texture, vec4 samples[NUM_SSAO_SAMPLES])
{
    // original normal of the pixel in world and view space
    vec3 world_normal = texture(normal_texture, uv).xyz;
    vec3 view_normal = (scene.world_to_view * vec4(world_normal, 0.0f)).xyz;

    // compute view position and depth at current pixel - would it be better to store this to speed up access?
    float z = texture(depth_texture, uv).r;
    vec4 clip_position = vec4(frag_position, z, 1.0f);
    vec4 view_position = scene.clip_to_view * clip_position;
    view_position /= view_position.w;

    // use the dither matrix to compute randomized normal, tangent, bitangent vectors
    // this is an alternative method to passing in a bunch of vectors in a uniform buffer
    ivec2 pixel_uv = pixelCoord(uv, scene.viewport_size);
    float dither = (dither_map_4[(pixel_uv.x % 4) + ((pixel_uv.y % 4) * 4)] / 16.0f) * 2.0f - 1.0f;
    vec3 view_perp = cross(view_normal, vec3(0, 0, 1));
    vec3 view_perp2 = cross(view_perp, view_normal);

    vec3 view_tangent = normalize((sin(dither * 6.28f) * view_perp) + (cos(dither * 6.28f) * view_perp2));
    vec3 view_bitangent = cross(view_tangent, view_normal);
    view_tangent = normalize(cross(view_bitangent, view_normal));
    // finally, create a TBN matrix for the pixel
    mat3x3 tbn = mat3x3(view_tangent, view_bitangent, view_normal);

    // we accumulate occlusion from many samples
    float occlusion = 0.0f;
    for (int i = 0; i < NUM_SSAO_SAMPLES; ++i)
    {
        // offset according to the random sample direction and radius (and TBN)
        vec3 sample_position = view_position.xyz + ((tbn * samples[i].xyz) * radius);
        vec4 sample_clip_position = scene.view_to_clip * vec4(sample_position, 1.0f);
        sample_clip_position /= sample_clip_position.w;

        // take another sample of the depth at the resulting pixel
        float resample_z = texture(depth_texture, (sample_clip_position.xy * 0.5f) + 0.5f).r;
        vec4 resample_clip_position = vec4(sample_clip_position.xy, resample_z, 1.0f);
        // custom depth recalculation to skip as much math as we can, while still giving the correct value
        float resample_view_z = clip_to_view_depth(resample_clip_position);

        // if our origin pixel is possibly occluded by the geometry we encountered, add some occlusion
        if (resample_view_z >= sample_position.z + bias)
            occlusion += smoothstep(0.0f, 1.0f, radius / abs(resample_view_z - view_position.z));
    }

    // rescale occlusion and apply a power factor for visual niceness
    occlusion = pow(smoothstep(0.0f, 1.0f, 1.0f - (occlusion / NUM_SSAO_SAMPLES)), power);

    return occlusion;
}

// COLOUR CORRECTION =====================
vec3 gammaAdjust(vec3 value, float gamma, float exposure, float offset)
{
    return (exposure * pow(value + offset, vec3(gamma)));
}

vec3 sampleLut(vec3 colour, sampler3D lut)
{
    vec3 size = vec3(textureSize(lut, 0));
    float fract = (size.x - 1.0f) / size.x;
    float fract2 = 0.5f / size.x;
    vec3 linear = ((colour * fract) + fract2);
    return (texture(lut, linear).rgb);
}
