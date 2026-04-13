void vertex(in Vertex vert, inout vec4 pos, inout Varyings vars)
{
}

Fragment fragment(in Varyings vars)
{
    Fragment frag;
    frag.colour = vec4(1, 0, 1, 1);

    return frag;
}
