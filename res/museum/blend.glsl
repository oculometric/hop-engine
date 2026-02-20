#pragma DEFAULT_VERTEX

void vertex()
{
    #pragma CANVAS_TRANSFORM
}

#pragma CANVAS_ATTACHMENTS

layout(set = 2, binding = 0) uniform sampler2D original;
layout(set = 2, binding = 1) uniform sampler2D multiply;

void fragment()
{
    out_colour = vec4(texture(original, frag.uv).rgb * texture(multiply, frag.uv).rgb, 1);
}