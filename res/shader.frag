#version 450

layout(location = 0) in vec3 frag_position;
layout(location = 1) in vec3 frag_colour;
layout(location = 2) in vec3 frag_normal;
layout(location = 3) in vec3 frag_tangent;
layout(location = 4) in vec2 frag_uv;

layout(location = 0) out vec4 out_color;

layout(set = 2, binding = 0) uniform MaterialUniforms
{
    vec4 colour;
} material_uniforms;

layout(set = 2, binding = 1) uniform sampler2D tex2;

float saturate(float f) { return clamp(f, 0, 1); }

void main()
{
    out_color = vec4(texture(tex2, frag_position.xy).bgr * saturate(dot(frag_normal, -normalize(vec3(0.3f, 0.5, -1.0)))), 1);
}