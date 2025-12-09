layout(set = 0, binding = 0) uniform SceneUniforms
{
    mat4 world_to_view;
    mat4 view_to_clip;
    ivec2 viewport_size;
    float time;
    vec3 eye_position;
} scene;

layout(set = 1, binding = 0) uniform ObjectUniforms
{
    mat4 model_to_world;
    int id;
} object;

struct Frag
{
    vec4 position;
    vec4 colour;
    vec4 normal;
    vec4 tangent;
    vec2 uv;
};

#ifdef VERTEX
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 colour;
layout(location = 2) in vec4 normal;
layout(location = 3) in vec4 tangent;
layout(location = 4) in vec2 uv;

layout(location = 0) out Frag frag;
#endif

#ifdef FRAGMENT
layout(location = 0) in Frag frag;

layout(location = 0) out vec4 colour;
layout(location = 1) out vec4 normal;
layout(location = 2) out vec4 params;
layout(location = 3) out vec4 custom;
#endif

struct Light
{
    vec4 position;
    vec4 direction;
    vec4 colour;
    float spot_angle;
    float constant_attenuation;
    float linear_attenuation;
    float quadratic_attenuation;
    int light_type;
    bool enabled;
    ivec2 padding;
};

struct Material
{
    vec4 diffuse;
    vec4 specular;
    vec4 ambient;
    vec4 emissive;
    float specular_exponent;
    vec3 padding;
};