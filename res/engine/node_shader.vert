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
    frag.position = (object.model_to_world * vec4(in_position.xyz, 1));
    frag.colour = in_colour;
    frag.normal = in_normal;
    frag.tangent = in_tangent;
    frag.uv = in_uv;
    vec2 translation = vec2(scene.world_to_view[3][0], scene.world_to_view[3][1]);
    vec2 downsized_viewport = floor(scene.viewport_size.xy * 0.25f);
    translation = floor(translation * downsized_viewport) / downsized_viewport;
    mat4 translation_only = mat4(1, 0, 0, 0,
                                 0, 1, 0, 0,
                                 0, 0, 1, 0,
                                 translation.x, translation.y, 0.5f, 1);
    gl_Position = translation_only * ((object.model_to_world * vec4(in_position.xyz, 1) / vec4(scene.viewport_size.xy / 2.0f, 1, 1)) * vec4(1, -1, -1, 1));
    gl_Position.xy = floor(gl_Position.xy * floor(scene.viewport_size.xy * 0.5f)) / floor(scene.viewport_size.xy * 0.5f);
}