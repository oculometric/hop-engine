#pragma CANVAS_ATTACHMENTS

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
}

uniform MaterialUniforms
{
    vec3 colour_filter;
};

bool fragment(in Varyings vars, inout Fragment frag)
{
    if (length(vars.colour.rgb - colour_filter) < 0.01f)
        frag.colour = vec4(1);
    else
        frag.colour = vec4(vars.colour.rgb, 1);
    return true;
}