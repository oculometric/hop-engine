#version 450

layout(set = 0, binding = 0) uniform SceneUniforms
{
    mat4 world_to_view;
    mat4 view_to_clip;
    float time;
} scene_uniforms;

layout(set = 1, binding = 0) uniform ObjectUniforms
{
    mat4 model_to_world;
    int id;
} object_uniforms;

layout(set = 2, binding = 0) uniform MaterialUniforms
{
    vec4 colour;
} material_uniforms;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 colour;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec2 uv;

layout(location = 0) out vec3 frag_position;
layout(location = 1) out vec3 frag_colour;
layout(location = 2) out vec3 frag_normal;
layout(location = 3) out vec3 frag_tangent;
layout(location = 4) out vec2 frag_uv;

void main()
{
    frag_position = position;
    frag_colour = colour;
    frag_normal = normal;
    frag_tangent = tangent;
    frag_uv = uv;

    gl_Position = scene_uniforms.view_to_clip * scene_uniforms.world_to_view * object_uniforms.model_to_world * vec4(position.xyz, 1.0);
}