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
layout(set = 2, binding = 2) uniform sampler2D tex3;

void main()
{
    out_color = texture(tex2, frag_position.xy).bgra;
}