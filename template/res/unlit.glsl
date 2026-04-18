uniform sampler2D albedo;
// add textures here

uniform Uniforms
{
    vec4 material_colour;
    // add extra uniform variables here
};

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars /*, out CustomStruct cs*/ )
{
    // modify the vertex transform here
}

bool fragment(in Varyings vars, /*in CustomStruct cs,*/ out Fragment frag)
{
    // perform fragment shading here
    frag.colour.rgb = texture(albedo, vars.uv.xy).rgb * material_colour.rgb;
    return true; // return false to discard the pixel
}