struct Light
{
    vec4 position;
    vec4 direction;
    vec4 colour;
    float spot_angle;
    int type;
    bool enabled;
    float padding;
};

layout(set = 0, binding = 0) uniform SceneUniforms
{
    mat4 world_to_view;
    mat4 view_to_clip;
    mat4 clip_to_view;
    ivec2 viewport_size;
    vec3 eye_position;
    float time;
    vec2 near_far;
    Light lights[8];
    vec4 ambient_light;
} scene;

#ifndef OMIT_OBJECT_SET
layout(set = 1, binding = 0) uniform ObjectUniforms
{
    mat4 model_to_world;
    int id;
} object;
#endif

struct Frag
{
    vec4 position;
    vec4 colour;
    vec4 normal;
    vec4 tangent;
    vec2 uv;
};

#ifdef VERTEX
layout(location = 0) in vec4 in_position;
layout(location = 1) in vec4 in_colour;
layout(location = 2) in vec4 in_normal;
layout(location = 3) in vec4 in_tangent;
layout(location = 4) in vec2 in_uv;

layout(location = 0) out Frag frag;
#endif

#ifdef FRAGMENT
layout(location = 0) in Frag frag;

#ifndef NONSTANDARD_FRAG_OUT
layout(location = 0) out vec4 out_colour;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_params;
layout(location = 3) out vec4 out_custom;
#endif
#endif

vec3 toLinear(vec3 srgb)
{
    return (pow(srgb, vec3(1.0f / 2.4f)) * 1.055f) -0.055f;
}