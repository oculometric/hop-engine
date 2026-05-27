uniform sampler2D image;

uniform Material
{
    vec4 colour;
};

bool fragment(in Varyings vars, inout Fragment frag)
{
    vec4 tex_colour = texture(image, vars.uv.xy) * colour;
    if (tex_colour.a < 0.5f)
        return false;
    frag.colour = vec4(tex_colour.rgb, 1.0f);
    return true;
}