#pragma DEFAULT_VERTEX

void vertex()
{
    #pragma DEFAULT_TRANSFORM
    frag.colour = in_colour;
}

#pragma DEFAULT_ATTACHMENTS

layout(set = 2, binding = 0) uniform sampler2D text_atlas;

void fragment()
{
    vec2 uv = frag.uv;
    if (texture(text_atlas, uv).r < 0.5f)
        discard;

    out_colour = vec4(frag.colour.rgb, 1);
    out_normal = vec4(0, 0, 0, 1);
    out_custom = vec4(0, 0, 0, 1);
    out_params = vec4(0, 0, 0, 0);
}