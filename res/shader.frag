#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform MaterialUniforms
{
    vec4 colour;
} material_uniforms;

void main()
{
    outColor = vec4(fragColor, 1.0);// * material_uniforms.colour;
}