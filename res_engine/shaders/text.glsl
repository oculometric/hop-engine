void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
}

uniform sampler2D text_atlas;

bool fragment(in Varyings vars, inout Fragment frag)
{
    vec2 uv = vars.uv.xy;
    if (texture(text_atlas, uv).r < 0.5f)
        return false;

    frag.colour = vec4(vars.colour.rgb, 1);
    return true;
}