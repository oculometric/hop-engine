#pragma DEFAULT_VERTEX

void vertex()
{
    #pragma CANVAS_TRANSFORM
    gl_Position = scene.view_to_clip * vec4(in_position.xyz, 1);
}

#pragma CANVAS_ATTACHMENTS

layout(set = 2, binding = 0) uniform sampler2D screen_texture;
layout(set = 2, binding = 1) uniform sampler2D stencil_texture;

layout(set = 2, binding = 2) uniform MaterialUniforms
{
    int display_depth;
};

void fragment()
{
    if (display_depth == 1)
    {
        float d = texture(screen_texture, frag.uv.xy).r;
        float depth = scene.near_far.x * scene.near_far.y / (scene.near_far.y + d * (scene.near_far.x - scene.near_far.y));
        out_colour = vec4(vec3(depth), 1);
    }
    else if (display_depth == 2)
    {
        float d = texture(stencil_texture, frag.uv.xy).r * 16.0f;
        out_colour = vec4(vec3(d), 1);
    }
    else
    out_colour = vec4(texture(screen_texture, frag.uv.xy).rgb, 1);
}