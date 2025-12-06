#version 450

#define FRAGMENT
#include "common.glsl"

layout(set = 2, binding = 0) uniform MaterialUniforms
{
    vec4 colour;
} material_uniforms;

layout(set = 2, binding = 1) uniform sampler2D tex2;

float saturate(float f) { return clamp(f, 0, 1); }

void main()
{
    colour = vec4(texture(tex2, frag.position.xy).rgb * saturate(dot(frag.normal, -normalize(vec3(0.3f, 0.5, -1.0)))), 1);
}