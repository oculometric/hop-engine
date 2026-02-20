#pragma DEFAULT_VERTEX

void vertex()
{
    #pragma DEFAULT_TRANSFORM
}

#pragma CANVAS_ATTACHMENTS

layout(set = 2, binding = 0) uniform MaterialUniforms
{
    vec3 colour_filter;
};

void fragment()
{
    if (length(frag.colour.rgb - colour_filter) < 0.01f)
        out_colour = vec4(1);
    else
        out_colour = vec4(frag.colour.rgb, 1);
}