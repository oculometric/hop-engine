#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform MaterialUniforms
{
    vec4 colour;
} material_uniforms;

layout(set = 2, binding = 1) uniform sampler2D tex2;

void main()
{
    outColor = texture(tex2, fragUV);//vec4(fragColor, 1.0);// * material_uniforms.colour;
}