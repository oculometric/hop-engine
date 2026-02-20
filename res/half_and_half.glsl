#pragma DEFAULT_VERTEX

void vertex()
{
    #pragma CANVAS_TRANSFORM
}

#pragma CANVAS_ATTACHMENTS

layout(set = 2, binding = 0) uniform sampler2D tex_left;
layout(set = 2, binding = 1) uniform sampler2D tex_right;
layout(set = 2, binding = 2) uniform sampler2D tex_third;

void fragment()
{
    if (frag.uv.x > 0.666f && frag.uv.y < 0.333f)
        out_colour = vec4(texture(tex_right, frag.uv * 3.0f).rgb, 1);
    else if (frag.uv.x < 0.333f && frag.uv.y < 0.333f)
        out_colour = vec4(texture(tex_third, frag.uv * 3.0f).rgb, 1);
    else
        out_colour = vec4(texture(tex_left, frag.uv).rgb, 1);
}