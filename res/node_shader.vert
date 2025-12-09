#version 450

#define VERTEX
#include "common.glsl"

void main()
{
    frag.position = (object.model_to_world * vec4(position, 1)).xyz;
    frag.colour = colour;
    frag.normal = normal;
    frag.tangent = tangent;
    frag.uv = uv;
    mat4 translation_only = mat4(1, 0, 0, 0,
                                 0, 1, 0, 0,
                                 0, 0, 1, 0,
                                 scene.world_to_view[3][0], scene.world_to_view[3][1], 0.5f, 1);
    gl_Position = translation_only * ((object.model_to_world * vec4(position.xyz, 1)
        / vec4(scene.viewport_size.xy / 2.0f, 1, 1))
        * vec4(1, -1, -1, 1));
}