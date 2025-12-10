#version 450

#define VERTEX
#include "common.glsl"

layout(set = 2, binding = 0) uniform MaterialUniforms
{
    bool debug_segments;
    int background_mode;
    float background_factor;
    vec4 background_colour;
};

void main()
{
    frag.position = (object.model_to_world * vec4(position.xyz, 1));
    frag.colour = colour;
    frag.normal = normal;
    frag.tangent = tangent;
    frag.uv = uv;
    vec2 translation = vec2(scene.world_to_view[3][0], scene.world_to_view[3][1]);
    translation = floor(translation * scene.viewport_size.xy * 0.25f) / (scene.viewport_size.xy * 0.25f);
    mat4 translation_only = mat4(1, 0, 0, 0,
                                 0, 1, 0, 0,
                                 0, 0, 1, 0,
                                 translation.x, translation.y, 0.5f, 1);
    gl_Position = translation_only * ((object.model_to_world * vec4(position.xyz, 1) / vec4(scene.viewport_size.xy / 2.0f, 1, 1)) * vec4(1, -1, -1, 1));
}