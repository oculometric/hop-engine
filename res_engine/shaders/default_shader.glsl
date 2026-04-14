void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
}

bool fragment(in Varyings vars, out Fragment frag)
{
    frag.colour = vec4(1, 0, 1, 1);
    return true;
}
