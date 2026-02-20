#pragma DEFAULT_VERTEX

void vertex()
{
    #pragma CANVAS_TRANSFORM
}

#pragma CANVAS_ATTACHMENTS

layout(set = 2, binding = 0) uniform sampler2D main_tex;
layout(set = 2, binding = 1) uniform sampler2D tex_1;
layout(set = 2, binding = 2) uniform sampler2D tex_2;
layout(set = 2, binding = 3) uniform sampler2D tex_3;
layout(set = 2, binding = 4) uniform sampler2D tex_4;
layout(set = 2, binding = 5) uniform sampler2D tex_5;
layout(set = 2, binding = 6) uniform sampler2D tex_6;
layout(set = 2, binding = 7) uniform sampler2D tex_7;

float lineariseDepth(float d)
{
    return (scene.near_far.x * scene.near_far.y / (scene.near_far.y + d * (scene.near_far.x - scene.near_far.y))) / scene.near_far.y;
}

void fragment()
{
    if (frag.uv.y < 0.125f)
    {
        if (frag.uv.x < 0.125f)
            out_colour = texture(tex_1, frag.uv * 8.0f);
        else if (frag.uv.x < 0.25f)
            out_colour = texture(tex_2, (frag.uv * 8.0f) - vec2(1, 0));
        else if (frag.uv.x < 0.375f)
            out_colour = texture(tex_3, (frag.uv * 8.0f) - vec2(2, 0));
        else if (frag.uv.x < 0.5f)
            out_colour = texture(tex_4, (frag.uv * 8.0f) - vec2(3, 0));
        else if (frag.uv.x < 0.625f)
            out_colour = vec4(vec3(lineariseDepth(texture(tex_5, (frag.uv * 8.0f) - vec2(4, 0)).r)), 1.0f);
        else if (frag.uv.x < 0.75f)
            out_colour = texture(tex_6, (frag.uv * 8.0f) - vec2(5, 0));
        else if (frag.uv.x < 0.875f)
            out_colour = texture(tex_7, (frag.uv * 8.0f) - vec2(6, 0));
        else
            out_colour = texture(main_tex, frag.uv);
    }
    else
        out_colour = texture(main_tex, frag.uv);
}