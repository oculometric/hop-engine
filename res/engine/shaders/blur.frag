#version 450

#define FRAGMENT
#define NONSTANDARD_FRAG_OUT
#define OMIT_OBJECT_SET
#include "common.glsl"

layout(location = 0) out vec4 out_colour;

layout(set = 2, binding = 0) uniform sampler2D main_tex;

mat3 gaussian_kernel = mat3(
    1, 2, 1,
    2, 4, 2,
    1, 2, 1) / 16.0f;

float[25] gaussian_kernel_5 =
float[](
    1, 2, 4, 2, 1,
    2, 4, 8, 4, 2,
    4, 8, 16, 8, 4,
    2, 4, 8, 4, 2,
    1, 2, 4, 2, 1
);

float[81] gaussian_kernel_9 =
float[](
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

void main()
{
    evaluateKernel(gaussian_kernel_9, 9, main_tex, out_colour);
}